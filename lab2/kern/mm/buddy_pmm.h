#ifndef __KERN_MM_BUDDY_PMM_H__
#define  __KERN_MM_BUDDY_PMM_H__

#include <pmm.h>

extern const struct pmm_manager buddy_pmm_manager;

#define MAX_BUDDY_ORDER 20
/* buddy system 的结构体 */
typedef struct {
    unsigned int max_order;                           // 实际最大块的大小
    list_entry_t free_array[MAX_BUDDY_ORDER + 1];     // 伙伴堆数组
    unsigned int nr_free;                             // 伙伴系统中剩余的空闲块
} free_buddy_t;

#endif /* ! __KERN_MM_BUDDY_PMM_H__ */