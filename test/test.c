#include "page.h"
#include "slab.h"
#include "mm.h"
#include "kmalloc.h"
#include "printk.h"
#include "lib.h"
#include "system.h"
#include "config.h"
#include "fork.h"
#include "sched.h"


extern unsigned char OS_RUNNING;
extern struct task_struct *tmp_test_create_thread(int (*f)(void *), void *args);
extern struct mm_struct *mm_alloc(void);
extern void schedule(void);
extern void show_free_areas(void);
extern void proc_caches_init(void);
extern void cpu_arm920_cache_clean_invalidate_all(void);
extern unsigned long MMU_TLB_BASE;
extern void cpu_arm920_tlb_invalidate_all(void);
unsigned long sys_brk(unsigned long brk);

void test_mm()
{
    int *p;
    int i;
    struct page *page0;
    unsigned char *test_kmalloc;
    kmem_cache_t *test_cache;

    page0 = alloc_pages(0, 0);
    printk("page_to_pfn << PAGE_SHIFT: %#lx\n", (__page_to_pfn(page0)) << PAGE_SHIFT);
    page0 = alloc_pages(0, 0);
    printk("page_to_pfn << PAGE_SHIFT: %#lx\n", (__page_to_pfn(page0)) << PAGE_SHIFT);
    page0 = alloc_pages(0, 8);
    printk("page_to_pfn << PAGE_SHIFT: %#lx\n", (__page_to_pfn(page0)) << PAGE_SHIFT);
    page0 = alloc_pages(0, 8);
    printk("page_to_pfn << PAGE_SHIFT: %#lx\n", (__page_to_pfn(page0)) << PAGE_SHIFT);
    page0 = alloc_pages(0, 0);
    printk("page_to_pfn << PAGE_SHIFT: %#lx\n", (__page_to_pfn(page0)) << PAGE_SHIFT);
    show_free_areas();

    printk("page - mem_map: %d\n", page0 - mem_map);
    printk("page_to_pfn: %ld\n", __page_to_pfn(page0));
    printk("page_to_pfn << PAGE_SHIFT: %#lx\n", (__page_to_pfn(page0)) << PAGE_SHIFT);

    void *addr = page_address(page0);
    printk("addr: %p\n", addr);

    test_cache = kmem_cache_create("hello", 32, 0, 0, NULL, NULL);

    test_kmalloc = kmalloc(32, 0);
    kfree(test_kmalloc);

    p = kmem_cache_alloc(test_cache, SLAB_KERNEL);
    p = kmem_cache_alloc(test_cache, SLAB_KERNEL);
    p = kmem_cache_alloc(test_cache, SLAB_KERNEL);
    p = kmem_cache_alloc(test_cache, SLAB_KERNEL);
    kmem_cache_free(test_cache, p);

    show_free_areas();

    for (i = 0; i < 65536; i++)
    {
        page0 = alloc_pages(0, 0);
        printk("%p %p\n", page0, page0->virtual);
        show_free_areas();
    }
}

extern void cpu_arm920_set_pgd(unsigned long pgd);


unsigned long L1_BASE;
unsigned long L2_BASE;

#define COARSE_PAGE                         0x02
#define L2_DESC_FULL_ACCESS                 0xff0
#define L2_DESC_OFFSET_MASK                 0xff000
#define L2_DESC_NCNB                        0x00
#define L2_DESC_CB                          ((1 << 2) | (1 << 3))
#define L2_DESC_CNB                         (1 << 3)
#define L2_DESC_SET_DOMAIN(vaddr, domain)   ((vaddr) |= (((domain) & 0xf) << 5))
#define L2_DESC_OFFSET_IN_SEC(vaddr)        (((vaddr) & L2_DESC_OFFSET_MASK) >> 12)

inline void set_l1_desc(unsigned long l1_base, unsigned long vaddr, unsigned long desc)
{
    volatile unsigned long *ptr = (volatile unsigned long *)l1_base;

    //*(ptr + (vaddr >> 20)) = desc;
	ptr += vaddr >> 20;
	*ptr = desc;
}

inline void set_l2_desc(unsigned long l2_base, unsigned long vaddr, unsigned long paddr)
{
    volatile unsigned long *ptr;

    ptr = (volatile unsigned long *)l2_base;
    ptr += (vaddr & 0xff000) >> 12;
    *ptr = paddr;
}

inline int arch_mmap(unsigned long vaddr, unsigned long paddr, unsigned long size, unsigned long attr)
{
    set_l2_desc(L2_BASE, vaddr, paddr | 0xff0 | 1 << 2 | 1 << 3 | 0x02);
    set_l1_desc(L1_BASE, vaddr, L2_BASE | (3 << 5) | 0x01);

    return 0;
}

extern void sys_fork(struct pt_regs *);
int test_swich_mm_task0(void *arg)
{
    struct pt_regs reg;
    int ret = 0;

    //do_brk(0x40000000, 0x1000);
    //do_brk(0x90001000, 0x1000);
    
    //ret = do_fork(0, 0, 0, 0x2000);

    sys_fork(&reg);

    if (ret > 0)
        printk("parent\n");
    else if (ret == 0)
        printk("chiled\n");
    
    while (1)
    {
        printk("1234\n");
        schedule();
    }

    return 0;
}

int test_swich_mm_task1(void *arg)
{
    int ret;

    //ret = do_fork(0, 0, 0, 0x2000);

    if (ret > 0)
        *(volatile unsigned int *)0x40000000 = 0x55;
    else if (ret == 0)
        *(volatile unsigned int *)0x40000000 = 0x1234;

    while (1)
    {
        printk("task 1 0x40000000: %x\n", *(volatile unsigned int *)0x40000000);
        schedule();
    }

    return ret;
}

void do_switch_mm(struct mm_struct *mm)
{
    cpu_arm920_set_pgd((unsigned long )mm->pgd);
    
    *(volatile unsigned int *)0x80000000 = 0xaa;
}

volatile unsigned char hole[0x100000];
struct task_struct *test_task_struct[2];

void tmp_add_task_struct(struct task_struct *tsk)
{
    test_task_struct[1] = tsk;
}

struct task_struct * tmp_get_next_ready(void)
{
    static unsigned int i = 0;

    i++;
#if 0
    printk("%d %x %x\n", 0, test_task_struct[0], test_task_struct[0]->mm);
    printk("%d %x %x\n", 1, test_task_struct[1], test_task_struct[1]->mm);
    printk("%d %x %x\n", i, test_task_struct[i & 0x01], test_task_struct[i & 0x01]->mm);
    printk("%d %x %x %x\n", i, test_task_struct[i & 0x01], test_task_struct[i & 0x01]->mm, test_task_struct[i & 0x01]->mm->pgd);
#endif
    //do_switch_mm(test_task_struct[i & 0x01]->mm);

    return test_task_struct[i & 0x01];
}

struct mm_struct *test_switch_mm_alloc_mm(void)
{
   struct mm_struct* mm = mm_alloc();
   mm->mmlist.next = &mm->mmlist;
   mm->mmlist.prev = &mm->mmlist;
    
   memcpy((void *)mm->pgd, (void *)TLB_BASE, 4096 * 4);

   return mm;
}

void test_switch_mm(void)
{
    void *addr;
    struct page *p;
    struct mm_struct *mm;

    current = test_task_struct[0];
    test_task_struct[0] = tmp_test_create_thread(test_swich_mm_task0, NULL);
//    mm = test_switch_mm_alloc_mm();
 //   test_task_struct[0]->mm = mm;

#if 0 
    p = alloc_pages(GFP_KERNEL, 0);
    addr = (void *)page_address(p);

    L1_BASE = (unsigned long)test_task_struct[0]->mm->pgd;
    L2_BASE = (unsigned long)addr;
    
    arch_mmap(0x40000000, 0x32001000, 0x1000, MMU_SECDESC_WB_NCNB);
#endif

#if 0
    test_task_struct[1] = tmp_test_create_thread(test_swich_mm_task1, NULL);
    mm = test_switch_mm_alloc_mm();
    test_task_struct[1]->mm = mm;
#if 1
    p = alloc_pages(GFP_KERNEL, 0);
    addr = (void *)page_address(p);

    L1_BASE = (unsigned long)test_task_struct[1]->mm->pgd;
    L2_BASE = (unsigned long)addr;
    
    arch_mmap(0x40000000, 0x32002000, 0x2000, MMU_SECDESC_WB_NCNB);
#endif
    
    printk("%x %x\n", test_task_struct[0], test_task_struct[0]->mm);
    printk("%x %x\n", test_task_struct[1], test_task_struct[1]->mm);

    printk("%x %x\n", test_task_struct[0], test_task_struct[0]->mm);
    printk("%x %x\n", test_task_struct[1], test_task_struct[1]->mm);
#endif

    OS_RUNNING = 1;
    do_switch_mm(test_task_struct[0]->mm);
    OS_Start();
}


struct task_struct test_current;
void test_do_brk(void)
{

    test_switch_mm();
    struct mm_struct *mm_test;
    
    current = &test_current;
    memset(current, 0, sizeof (*current));

    mm_test = mm_alloc();
   
    current->mm = mm_test;
    current->mm->mmlist.next = &current->mm->mmlist;
    current->mm->mmlist.prev = &current->mm->mmlist;

    printk("curretn: %x current->mm: %x mm_test: %x\n", current, current->mm, mm_test);

    sys_brk(4096);
    show_mm(current->mm);
    sys_brk(8192);
    show_mm(current->mm);
    sys_brk(4096);
    show_mm(current->mm);
    do_brk(0x30000000, 81920);
    show_mm(current->mm);

}

void test_l2_mmap_set_pgd(void)
{

    test_do_brk();

    struct page *page_tlb;
    struct page *page_l2;
    void *addr0, *addr1;

    *(volatile unsigned int *)0x32000000 = 0xaa;

    page_tlb = alloc_pages(0, 8);
    addr0 = (void *)page_address(page_tlb);
    L1_BASE = (unsigned long)addr0;
    
    page_l2 = alloc_pages(0, 6);
    addr1 = (void *)page_address(page_l2);
    L2_BASE = (unsigned long)addr1;
    
    
    memcpy(addr0, (void *)TLB_BASE, 0x10000);
    
    arch_mmap(0x32000000, 0x32001000, 0x1000, MMU_SECDESC_WB_NCNB);

    MMU_TLB_BASE = (unsigned long)L1_BASE;
    
    cpu_arm920_cache_clean_invalidate_all();
    cpu_arm920_tlb_invalidate_all();
    cpu_arm920_set_pgd((unsigned long)addr0);
    cpu_arm920_cache_clean_invalidate_all();
    cpu_arm920_tlb_invalidate_all();

    *(volatile unsigned int *)0x32000000 = 0x55;

    MMU_TLB_BASE = (unsigned long)TLB_BASE;
    cpu_arm920_tlb_invalidate_all();
    cpu_arm920_cache_clean_invalidate_all();
    cpu_arm920_set_pgd(TLB_BASE);
    cpu_arm920_cache_clean_invalidate_all();
    cpu_arm920_tlb_invalidate_all();
    
    MMU_TLB_BASE = (unsigned long)L1_BASE;
    cpu_arm920_cache_clean_invalidate_all();
    cpu_arm920_tlb_invalidate_all();
    cpu_arm920_set_pgd((unsigned long)addr0);
    cpu_arm920_cache_clean_invalidate_all();
    cpu_arm920_tlb_invalidate_all();

    printk("set pgd OK\n");

}

void test_set_pgd(void)
{
    struct page *page_tlb;
    void *addr;
    
    test_l2_mmap_set_pgd();

    page_tlb = alloc_pages(0, 4);
    addr = (void *)page_address(page_tlb);


    memcpy(addr, (void *)TLB_BASE, 0x10000);

    cpu_arm920_set_pgd((unsigned long)addr);

    printk("set pgd OK\n");
}