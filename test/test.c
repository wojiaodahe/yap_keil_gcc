#include "page.h"
#include "slab.h"
#include "mm.h"
#include "kmalloc.h"
#include "printk.h"
#include "lib.h"
#include "system.h"
#include "config.h"

#include "sched.h"

void test_mm()
{
    int *p;
    int i;
    struct page *page0;
    unsigned char *test_kmalloc;
    struct mm_struct *mm_test;
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
    set_l2_desc(L2_BASE, vaddr, paddr | 0xff0 | 1 << 2 | 1 <<3 | 0x02);
    set_l1_desc(L1_BASE, vaddr, L2_BASE | (3 << 5) | 0x01);

    return 0;
}


extern void proc_caches_init(void);
extern void cpu_arm920_cache_clean_invalidate_all(void);
extern unsigned long MMU_TLB_BASE;
extern void cpu_arm920_tlb_invalidate_all(void);
unsigned long sys_brk(unsigned long brk);

struct task_struct test_current;
void test_do_brk(void)
{
    struct mm_struct *mm_test;
    
    current = &test_current;
    memset(current, 0, sizeof (*current));

    proc_caches_init();
    
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