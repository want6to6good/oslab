## 操作系统实验报告 lab3

### 练习1:理解基于 FIFO 的页面替换算法

   - 1._fifo_init_mm：用于初始化FIFO页面置换算法所需的数据结构。它首先使用list_init函数来初始化pra_list_head，然后将mm->sm_priv字段设置为指向pra_list_head的地址，从而将FIFO页面置换算法与内存管理结构(mm)相关联。
   
   - 2._fifo_map_swappable：根据FIFO算法将最近访问的页面链接到pra_list_head队列的末尾。它首先获取pra_list_head队列的头部和要添加的页面的链接，然后使用list_add函数将页面添加到队列的末尾。
   
   - 3._fifo_swap_out_victim：根据FIFO算法选择需要被替换的页面。它首先检查队列是否为空，然后使用list_prev函数获取队列中最早到达的页面，将其从队列中移除，最后将此页面的地址设置为ptr_page。
   
   - 4._fifo_check_swap：这是用于检查FIFO页面置换算法正确性的测试函数。它模拟页面错误并根据FIFO算法的规则记录页面访问事件。

   - 5._fifo_init：初始化FIFO页面置换管理器。

   - 6._fifo_set_unswappable：将指定地址的页面标记为不可交换，通常用于内核空间的页面。

   - 7._fifo_tick_event：通常在时钟中断处理中被调用，处理页面置换相关事件。

   - 8.list_init 和其他与链表相关的函数/宏：这些函数/宏来自list.h，用于管理双向链表数据结构（pra_list_head）。它们用于初始化、添加、删除和操作FIFO算法中的链表条目。

   - 9.assert：assert函数用于调试和验证。它确保满足某些条件，用于检查_fifo_map_swappable和_fifo_swap_out_victim函数的算法正确性。

   - 10.标准库函数，如cprintf和内存访问函数：虽然不是页面置换算法本身的直接部分，但这些函数用于打印调试信息和在测试阶段访问内存。

### 练习2:深入理解不同分页模式的工作原理

   
   - 1.相似之处：
	目标: 无论采用哪种分页模式，这两段代码的目标都是获取给定线性地址（la）对应的页表项（PTE）的指针。
	PDT 结构: 无论分页模式如何，都有两层页表结构，即一级页表和二级页表。
	递归查找: 两段代码都执行递归查找，首先查找一级页表中的 PTE，然后查找二级页表中的 PTE。
	PTE 存在性检查: 两段代码都会检查所访问的 PTE 是否存在（PTE_V 标志），以确保它们在物理内存中存在。
       
       不同之处：
	分页模式的不同: 两段代码的主要不同之处在于它们针对不同的分页模式，如 sv32、sv39 和 sv48，因此它们的页表层次和位宽度可能会有所不同。sv32 使用 2 级页表，sv39 使用 3 级页表，而 sv48 使用 4 级页表。
	页表项宽度: 不同分页模式的页表项可能有不同的宽度，例如，sv32 使用 32 位的页表项，而 sv39 和 sv48 使用 64 位的页表项。
	页表项结构: sv32 和 sv39 在页表项结构方面有些许不同，而 sv48 与之更为不同。这涉及到位字段的偏移和宽度，以及它们的含义（如 PTE_V、PTE_R 等）。
	物理地址位数: 不同的分页模式允许不同数量的物理地址位。sv32 允许 32 位物理地址，而 sv39 和 sv48 支持更大的物理地址空间。
  
   - 2.优点：
	简单性: 将PTE查找和分配合并在一个函数中可以使代码更加简单和易于理解。这减少了多个函数调用和分支逻辑的需要，让代码更加清晰。
	优化: 该函数可以通过仅在需要时分配页来优化内存管理。如果PTE已经存在，就没有必要分配新的页。这种优化可以提高性能。
	原子性: 通过将这些操作合并，确保了分配和更新PTE的原子性。这有助于在多线程或多处理器环境中保持一致性。
	可读性: 该函数清晰地表明其目标是获取PTE，如果不存在，可以选择创建一个。这种清晰性可以提高代码的可读性，拆分为不同的函数。
	模块化: 如果您想要更模块化的代码库，可以将PTE查找和分配分成不同的函数。这可以提供更多的灵活性，并更容易测试各个组件。
	自定义错误处理: 如果需要对PTE分配失败进行自定义错误处理或日志记录，拆分这些功能可能更容易实现。
	代码可重用性: 将这些功能分开可以使代码更容易在不同的上下文中重用。

### 练习3：给未被映射的地址映射上物理页

   - 1.Page Directory Entry 和 Page Table Entry 的用途：
	Page Directory Entry (PDE): PDE 用于构建页表。mm_struct 的 pgdir 指针指向页目录（Page Directory），其中每个 PDE 对应一个页表。通过修改 PDE，可以创建新的页表，或者访问现有的页表。这在页替换算法中很有用，因为可以更改页表的映射关系以实现页面置换。
	Page Table Entry (PTE): PTE 用于表示虚拟地址到物理地址的映射。每个 PTE 包含页面的物理地址和权限标志。通过修改 PTE，可以更新虚拟地址到物理地址的映射，包括创建新的映射，交换页面等。这对于处理页错误时分配物理页面以及在页面置换算法中切换映射非常重要。

   - 2.当缺页服务例程（page fault handler）在执行过程中访问内存时，如果出现页访问异常，硬件会执行以下步骤：
	暂停当前进程的执行。
	保存当前进程的状态（例如，CPU 寄存器和程序计数器）。
	根据异常类型，确定访问的地址是无效的（不存在于页表中）。
	向操作系统内核发送一个异常，通常是将控制权转移到缺页服务例程（在 ucore 中即 do_pgfault 函数）。
	操作系统内核会在 do_pgfault 函数中处理页面错误，这包括分配物理页面、更新页表、将页面加载到内存等。
	最后，恢复当前进程的状态，以便它可以继续执行。

   - 3.数据结构 Page 和 页表项的对应关系：
	struct Page 是用于管理物理页面的数据结构，它表示系统中的物理页。这个数据结构不直接与页表项相关，但它用于存储页面的内容和其他信息。
         每个 PTE 包含指向物理页面的物理地址，因此 PTE 与 struct Page 之间存在关系。do_pgfault 函数根据 PTE 中的信息，如交换条目，可能会将页面加载到 struct         Page 中。
         do_pgfault代码如下:
         
```yacas
  int
do_pgfault(struct mm_struct *mm, uint_t error_code, uintptr_t addr) {
    int ret = -E_INVAL;
    //try to find a vma which include addr
    struct vma_struct *vma = find_vma(mm, addr);
    pgfault_num++;
    //If the addr is in the range of a mm's vma?
    if (vma == NULL || vma->vm_start > addr) {
        cprintf("not valid addr %x, and  can not find it in vma\n", addr);
        goto failed;
    }
    uint32_t perm = PTE_U;//初始化为用户权限
    if (vma->vm_flags & VM_WRITE) {
        perm |= (PTE_R | PTE_W);//升级
    }
    addr = ROUNDDOWN(addr, PGSIZE);
    ret = -E_NO_MEM;
    pte_t *ptep=NULL;
    ptep = get_pte(mm->pgdir, addr, 1);  //(1) try to find a pte, if pte's
                                         //PT(Page Table) isn't existed, then
                                         //create a PT.
    if (*ptep == 0) {
        if (pgdir_alloc_page(mm->pgdir, addr, perm) == NULL) {
            cprintf("pgdir_alloc_page in do_pgfault failed\n");
            goto failed;
        }
    } else {
        if (swap_init_ok) {
            struct Page *page = NULL;
           if((ret=swap_in(mm,addr,&page))!=0){
            // swap_in返回的值部位0，表示换入失败，输出错误信息
                cprintf("swap_in in do_pgfault failed\n");
                goto failed;
           }
            //(2) According to the mm, addr AND page, setup the map of phy addr <---> logical addr
            // 让交换进来的page页和mm->pgdir对应addr的二级页表项建立映射关系，perm中记载了该二级页表的各个权限位
            page_insert(mm->pgdir,page,addr,perm);
            //(3) make the page swappable.
            // 当前page可交换，将其加入全局虚拟内存管理器的管理
            swap_map_swappable(mm,addr,page,1);
            page->pra_vaddr = addr;
        } else {
            cprintf("no swap_init_ok but ptep is %x, failed\n", *ptep);
            goto failed;
        }
   }
   ret = 0;
failed:
    return ret;
}

   ``` 


### 练习4：
   
   - 在clock算法中，我们通过给页表结构体中增加了一个visited变量，用来记录最近是否被访问过，当我们对一个页面进行初始化时，首先将其设置为1，在决定哪个页面需要被置换时，我们需要遍历整个双向链表，对于之前标志位为1的页面，则将其置为0，代表检测之后已经被重置，对于标志位为0的，则意味着最近没有被访问过，则将其置换到硬盘中，从而实现了clock算法。
代码如下：
 ```yacas
  static int _clock_init_mm(struct mm_struct *mm)
{     
     list_init(&pra_list_head);
     curr_ptr = &pra_list_head;
     mm->sm_priv = &pra_list_head;
     return 0;
}
static int _clock_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
{
    list_entry_t *entry=&(page->pra_page_link);
 
    assert(entry != NULL && curr_ptr != NULL);
    list_add_before(curr_ptr, entry);
    page->visited = 0;
    return 0;
}
static int _clock_swap_out_victim(struct mm_struct *mm, struct Page ** ptr_page, int in_tick)
{
    list_entry_t *head=(list_entry_t*) mm->sm_priv;
    assert(head != NULL);
    assert(in_tick==0);
    while (1) {
        if (curr_ptr == head)
            curr_ptr = curr_ptr->next;
        struct Page *p = le2page(curr_ptr, pra_page_link);
        if (p->visited == 1) {
            p->visited = 0;
            curr_ptr = curr_ptr->next;
        }
        else {
            *ptr_page = p;
            cprintf("curr_ptr 0xffffffff%08x\n", curr_ptr);
            list_del(curr_ptr);
            curr_ptr = curr_ptr->next;
            break;
        }
    }
    return 0;
}
   ``` 
   - 不同点1：页面选择策略：
	FIFO算法：FIFO算法是一种最简单的页面替换算法。它根据页面加载到内存的顺序来选择要替换的页面。最早进入内存的页面会最早被替换出去，因此FIFO算法总是选择最老的页面进行替换。
	Clock算法：Clock算法是一种改进的页面替换算法，它引入了"时钟"的概念。Clock算法会遍历内存中的页面，查看每个页面的"引用位"。如果页面的引用位为0，说明该页面在一定时间内没有被访问过，可以被替换。如果引用位为1，说明页面最近被访问过，将引用位重置为0，并将时钟指针移动到下一个页面。这个过程循环执行，直到找到一个合适的页面被替换。
   
   - 不同点2：效率和性能：
	FIFO算法：FIFO算法非常简单，FIFO总是替换最早进入内存的页面，而不考虑页面的使用频率。
	Clock算法：Clock算法通过考虑页面的使用频率来进行更智能的替换。Clock算法相对于FIFO来说更加复杂，因为它需要维护每个页面的引用位，并需要周期性地扫描所有页面。

### 练习5：
   - 优势和优势：
减少页表项数量： 一个大页的页表映射方式可以显著减少页表中的页表项数量。这意味着在管理虚拟内存时需要更少的页表项，减少了页表的内存占用和访问时间。
快速地址转换： 大页可以提高内存访问的速度。因为更多的虚拟地址空间可以映射到同一个物理页，这减少了对页表的多次查找。这种映射方式可以减少CPU在地址转换过程中的开销。
减少TLB缺失： 大页可以减少TLB缺失的频率。TLB是用于加速地址转换的高速缓存，如果一个大页中的多个虚拟地址都映射到同一个物理页，那么只需一次缓存页表项，这减少了TLB缺失的机会。
性能提升： 大页表映射方式通常可以提高内存访问速度，因为减少了页表的访问次数和TLB缺失次数。这在一些应用程序和工作负载下可以提供显著的性能提升。

   - 坏处和风险：
内部碎片： 使用大页可能会导致内部碎片的浪费。如果一个大页中只有一小部分被使用，那么其余部分可能会浪费掉，这可能会导致内存资源浪费。
不适用于所有应用： 大页适用于某些类型的应用，但不适用于所有应用程序。对于某些应用程序，大页表映射方式可能不会提供显著的性能改进，甚至可能会导致性能下降。
页表更新复杂性： 在采用大页表映射方式时，页表的更新可能变得更加复杂。如果需要更新一个虚拟地址所映射的物理页，可能需要更新大页中的多个虚拟地址。
硬件支持需求： 大页通常需要硬件支持，包括支持大页表映射的处理器和操作系统。如果硬件或操作系统不支持大页，那么无法使用这种映射方式。

### 拓展：
   - 在lru算法中，整体实现与clock较为相似，同样是以visited标志位来判断最近有没有被访问过，但是区别在于，在执行置换函数之前，我们要对整个双向链表进行遍历，对于每一个页表项都要将其与标志位做与操作，以判断最近有没有被访问过，如果有，就将其visited重置为0，如果没有，将其visited++，遍历完整个链表后，从中找出visited值最大的一个页表项进行置换，从而达到了最久没有被访问的页面优先被置换。对于检查函数，我直接照搬clock的，经测试通过。
代码如下：
```yacas
  #include <defs.h>
#include <riscv.h>
#include <stdio.h>
#include <string.h>
#include <swap.h>
#include <swap_lru.h>
#include <list.h>

list_entry_t pra_list_head;

static int _lru_init_mm(struct mm_struct *mm)
{
    list_init(&pra_list_head);
    mm->sm_priv = &pra_list_head;
    return 0;
}
static int _lru_check(struct mm_struct *mm)
{
    list_entry_t *head = (list_entry_t *)mm->sm_priv;   //头指针
    assert(head != NULL);
    list_entry_t *entry = head;
    while (1)
    {
        entry = list_prev(entry)
        if(entry==head)
            break;
        struct Page *entry_page = le2page(entry, pra_page_link);
        pte_t *tmp_pte = get_pte(mm->pgdir, entry_page->pra_vaddr, 0);
        if (*tmp_pte & PTE_A)  
        {
            entry_page->visited = 0;
            *tmp_pte = *tmp_pte ^ PTE_A;//清除访问位
        }
        else
            entry_page->visited++;
    }
}
static int _lru_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in)
{
    list_entry_t *entry = &(page->pra_page_link);
    assert(entry != NULL);
    list_entry_t *head = (list_entry_t *)mm->sm_priv;
    list_add(head, entry); // 将页面page插入到页面链表pra_list_head的末尾
    page->visited = 0;     //标记为未访问
    return 0;
}
static int _lru_swap_out_victim(struct mm_struct *mm, struct Page **ptr_page, int in_tick)
{
    _lru_check(mm);
    
    list_entry_t *head = (list_entry_t *)mm->sm_priv;
    assert(head != NULL);
    assert(in_tick == 0);
    list_entry_t *entry = list_prev(head);
    list_entry_t *temp = entry;
    uint_t largest_visted = 0;
    while (1)
    {
        // 遍历找到最大的visited，表示最早被访问的
        if (entry == head)
            break;
        if (le2page(entry, pra_page_link)->visited > largest_visted)
        {
            largest_visted = le2page(entry, pra_page_link)->visited;
            temp = entry;
        }
        entry = list_prev(entry);
    }
    list_del(temp);
    *ptr_page = le2page(temp, pra_page_link);
    //cprintf("curr_ptr %p\n", temp);
    return 0;
}
   ``` 