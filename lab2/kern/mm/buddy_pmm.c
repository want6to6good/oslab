#include <pmm.h>
#include <list.h>
#include <string.h>
#include <buddy_pmm.h>

free_buddy_t buddy_s;

#define buddy_array (buddy_s.free_array)
#define max_order (buddy_s.max_order)
#define nr_free (buddy_s.nr_free)
//判断是否为2的幂次方
static int is_pow2(size_t n){
    if (n & (n - 1))
        return 0;
    else
        return 1;
}
//返回一个和n最近的2的幂次方的指数
static unsigned int getOrderOf2(size_t n){
    unsigned int order = 0;
    while (n >>=1)
        order ++;
    return order;
}
//返回一个比n小且最近的2的幂次方
static size_t ROUNDDOWN2(size_t n){
    size_t res = 1;
    if (!is_pow2(n))
    {
        while (n>>=1)
            res = res << 1;
        return res;
    }
    else
        return n;
}
//返回一个比n大且最近的2的幂次方
static size_t ROUNDUP2(size_t n){
    size_t res = 1;
    if (!is_pow2(n))
     {
        while (n>>=1)
            res = res << 1;
 
        return res<<1;
    }
    else
        return n;
}

//初始化buddy结构体
static void
buddy_init(void)
{
    // 初始化链表数组中的每个free_list头
    for (int i = 0;i < MAX_BUDDY_ORDER;i ++)
        list_init(buddy_array + i);
    max_order = 0;
    nr_free = 0;
    return;
}
// 获取以page页为头页的块的伙伴块
static struct Page*
buddy_get_buddy(struct Page *page)
 {
    unsigned int order = page->property;
    unsigned int buddy_ppn = first_ppn + ((1 << order) ^ (page2ppn(page) - first_ppn));
    //计算伙伴页的页号
    cprintf("[!]BS: Page NO.%d 's buddy page on order %d is: %d\n", page2ppn(page), order, buddy_ppn);
    if (buddy_ppn > page2ppn(page))
        return page + (buddy_ppn - page2ppn(page));
    else
        return page - (page2ppn(page) - buddy_ppn);
}
static void
buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    size_t pnum;
    unsigned int order;
    //向下取整分配内存
    pnum = ROUNDDOWN2(n);       // 将页数向下取整为2的幂
    //pnum = 8; // test!!!
    order = getOrderOf2(pnum);   // 求出页数对应的2的幂
    struct Page *p = base;
    // 初始化pages数组中范围内的每个Page
    for (; p != base + pnum; p ++)
     {
        assert(PageReserved(p));
        p->flags = p->property = 0;   // 全部初始化为非头页
        set_page_ref(p, 0);
    }
    max_order = order;
    nr_free += pnum;
    list_add(&(buddy_array[max_order]), &(base->page_link)); // 将第一页base插入数组的最后一个链表，作为初始化的最大块——16384,的头页
    base->property = max_order;                       // 将第一页base的property设为最大块的2幂
    return;
}    
// 内存块不够时，实行分裂
static void buddy_split(size_t n)
 {
    assert(n > 0 && n <= max_order);
    assert(!list_empty(&(buddy_array[n])));
    //cprintf("[!]BS: SPLITTING!\n");
    struct Page *page_a;
    struct Page *page_b;
    page_a = le2page(list_next(&(buddy_array[n])), page_link);
    //取出内存块
    page_b = page_a + (1 << (n - 1));
    //算出b的物理地址
    page_a->property = n - 1;
    page_b->property = n - 1;

    list_del(list_next(&(buddy_array[n])));
    list_add(&(buddy_array[n-1]), &(page_a->page_link));
    list_add(&(page_a->page_link), &(page_b->page_link));
    return;
}
static struct Page *
buddy_alloc_pages(size_t n)
{
    assert(n > 0);
    if (n > nr_free)
        return NULL;
    struct Page *page = NULL;
    //向上取整
    size_t pnum = ROUNDUP2(n);  // 处理所要分配的页数，向上取整至2的幂
    size_t order = getOrderOf2(pnum);  // 求出所需页数对应的幂pow
    while(1)
    {
        if (!list_empty(&(buddy_array[order])))
        {
            page = le2page(list_next(&(buddy_array[order])), page_link);
            list_del(list_next(&(buddy_array[order])));
            SetPageProperty(page); // 将分配块的头页设置为已被占用
            break;
        }
        else
        {
            int found = 0;
            for (int i = order; i < max_order + 1; i++)
            {
                // 找到pow后第一个非空链表，分裂空闲块
                if (!list_empty(&(buddy_array[i])))
                {
                    buddy_split(i);
                    if (i == order + 1)
                        found = 1;
                    break;
                };
            };
            if (!found)
                break;
        }
    };
    nr_free -= pnum;
    return page;
}
static void
buddy_free_pages(struct Page *base, size_t n) {
    assert(n > 0);
    unsigned int pnum = 1 << (base->property);
    assert(ROUNDUP2(n) == pnum);

    struct Page* left_block = base;
    struct Page* buddy = NULL;
    struct Page* tmp = NULL;

    buddy = buddy_get_buddy(left_block);
    list_add(&(buddy_array[left_block->property]), &(left_block->page_link));
    // 当伙伴块空闲，且当前块不为最大块时
    while (!PageProperty(buddy) && left_block->property < max_order) {
        if (left_block > buddy)
        { // 若当前左块为更大块的右块
            left_block->property = 0;
            ClearPageProperty(left_block);
            tmp = left_block;
            left_block = buddy;
            buddy = tmp;
        }
        list_del(&(left_block->page_link));    
        list_del(&(buddy->page_link));//buffy[i]
        left_block->property += 1;
        list_add(&(buddy_array[left_block->property]), &(left_block->page_link)); // 头插入相应链表
        buddy = buddy_get_buddy(left_block);
    }
    ClearPageProperty(left_block); // 将回收块的头页设置为空闲
    nr_free += pnum;
    cprintf("pnum%d", pnum);
    return;
}

static size_t
buddy_nr_free_pages(void)
{
    return nr_free;
}


static void
buddy_check(void) {

    // Check if pages are allocated and freed correctly
    struct Page* p0, * p1, * p2, * p3, * p4;
    p0 = p1 = p2 = p3 = p4 = NULL;

    // Allocate and free pages
    p0 = alloc_page();
    assert(p0 != NULL);
    assert(page_ref(p0) == 0);

    p1 = alloc_page();
    assert(p1 != NULL);
    assert(page_ref(p0) == 0);
    assert(page_ref(p1) == 0);

    p2 = alloc_page();
    assert(p2 != NULL);
    assert(page_ref(p0) == 0);
    assert(page_ref(p1) == 0);
    assert(page_ref(p2) == 0);

    // Ensure pages are allocated in correct order
    assert(p0 != p1 && p0 != p2 && p1 != p2);

    // Free pages
    int temp = p0->property;
    int origin = nr_free;
    cprintf("temp: %d / %d origin\n", temp,origin);
    free_page(p0);
    cprintf("nr_free %d", nr_free);
    assert(nr_free == temp+origin);

    free_page(p1);
    assert(nr_free == 2);

    free_page(p2);
    assert(nr_free == 3);

    // Try to allocate more pages than available
    assert(alloc_page() == NULL);

    // Allocate and free pages again
    p3 = alloc_page();
    assert(p3 != NULL);
    assert(nr_free == 2);

    p4 = alloc_page();
    assert(p4 != NULL);
    assert(nr_free == 1);

    // Ensure pages are allocated in correct order
    assert(p3 != p4);

    free_page(p3);
    assert(nr_free == 2);

    free_page(p4);
    assert(nr_free == 3);

    // Check if the free list is organized correctly
    size_t expected_nr_free = 0;
    for (int i = 0; i <= max_order; i++) {
        expected_nr_free += list_size(&(buddy_array[i])) * (1 << i);
    }
    assert(expected_nr_free == nr_free);

    // Check if page reference count is maintained correctly
    assert(page_ref(p0) == 0);
    assert(page_ref(p1) == 0);
    assert(page_ref(p2) == 0);
    assert(page_ref(p3) == 0);
    assert(page_ref(p4) == 0);

    cprintf("[buddy_check] All checks passed!\n");


}

}
// LAB2: below code is used to check the first fit allocation algorithm (your EXERCISE 1) 
// NOTICE: You SHOULD NOT CHANGE basic_check, default_check functions!
const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};

