#ifndef __KERNEL_MM_H__
#define __KERNEL_MM_H__

#include "page.h"
#include "fs.h"
#include "atomic.h"

struct page* alloc_pages(int gpf_mask, unsigned long order);
void free_pages(unsigned long addr, unsigned long order);
void __free_pages(struct page *page, unsigned long order);
void *alloc_bootmem_node(pg_data_t *pgdat, unsigned long bitmap_size);
struct page *__alloc_pages(zonelist_t *zonelist, unsigned long order);
unsigned long __get_free_pages(int gfp_mask, unsigned long order);


struct vm_area_struct
{
    struct mm_struct *vm_mm;
    unsigned long vm_start;
    unsigned long vm_end;

    struct vm_area_struct *vm_next;

    pgprot_t vm_page_prot;
	unsigned long vm_flags;

    struct vm_area_struct *vm_next_share;
    struct vm_area_struct *vm_pprev_share;

    struct vm_operations_struct *vm_ops;
    unsigned long vm_pgoff;
    struct file *vmfile;
    unsigned long vm_raend;
    void *vm_private_data;
};

struct vm_operations_struct
{
    void (*open)(struct vm_area_struct *area);
    void (*close)(struct vm_area_struct *area);
    struct page* (*nopage)(struct vm_area_struct *area, unsigned long address, int write_access);
};

struct mm_struct
{
    struct vm_area_struct *mmap;
    struct vm_area_struct *mmap_avl;
    struct vm_area_struct *mmap_cache;
    
    spinlock_t page_table_lock;
    
    unsigned long def_flags;

    pgd_t *pgd;
    atomic_t mm_users;
    atomic_t mm_count;
    struct list_head mmlist;
    int map_count;

    unsigned long start_code, end_code, start_data, end_data;
    unsigned long start_brk, brk, start_stack;
    unsigned long arg_start, arg_end, env_start, env_end;
    unsigned long rss, total_vm, locked_vm;

    unsigned long swap_cnt;
    unsigned long swap_address;

    //mm_context_t context;
};

#define __free_page(page) __free_pages((page), 0)
#define free_page(addr) free_pages((addr), 0)

#define VM_READ		0x00000001	/* currently active flags */
#define VM_WRITE	0x00000002
#define VM_EXEC		0x00000004
#define VM_SHARED	0x00000008

#define VM_MAYREAD	0x00000010	/* limits for mprotect() etc */
#define VM_MAYWRITE	0x00000020
#define VM_MAYEXEC	0x00000040
#define VM_MAYSHARE	0x00000080

#define VM_GROWSDOWN	0x00000100	/* general info on the segment */
#define VM_GROWSUP	0x00000200
#define VM_SHM		0x00000400	/* shared memory area, don't swap out */
#define VM_DENYWRITE	0x00000800	/* ETXTBSY on write attempts.. */

#define VM_EXECUTABLE	0x00001000
#define VM_LOCKED	0x00002000
#define VM_IO           0x00004000	/* Memory mapped I/O or similar */

#define VM_SEQ_READ	0x00008000	/* App will access data sequentially */
#define VM_RAND_READ	0x00010000	/* App will not benefit from clustered reads */

#define VM_DONTCOPY	0x00020000      /* Do not copy this vma on fork */
#define VM_DONTEXPAND	0x00040000	/* Cannot expand with mremap() */
#define VM_RESERVED	0x00080000	/* Don't unmap it from swap_out */

#define VM_DATA_DEFAULT_FLAGS	(VM_READ | VM_WRITE | VM_EXEC | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC)


#define VM_STACK_FLAGS	0x00000177

#define VM_READHINTMASK			(VM_SEQ_READ | VM_RAND_READ)
#define VM_ClearReadHint(v)		(v)->vm_flags &= ~VM_READHINTMASK
#define VM_NormalReadHint(v)		(!((v)->vm_flags & VM_READHINTMASK))
#define VM_SequentialReadHint(v)	((v)->vm_flags & VM_SEQ_READ)
#define VM_RandomReadHint(v)		((v)->vm_flags & VM_RAND_READ)

#define get_page(p)                 atomic_inc(&(p)->count)
#define put_page(p)                 __free_page(p)
#define put_page_testzero(p)        atomic_dec_and_test(&(p)->count)
#define page_count(p)               atomic_read(&(p)->count)
#define set_page_count(p, v)        atomic_set(&(p)->count, v)

#define get_page(p)                 atomic_inc(&(p)->count)
#define put_page(p)                 __free_page(p)
#define page_count(p)               atomic_read(&(p)->count)
#define set_page_count(p, v)        atomic_set(&(p)->count, v)

/* Page flag bit values */
#define PG_locked		    -2
#define PG_error		    -1
#define PG_referenced	    0
#define PG_uptodate		    1
#define PG_dirty		    2
#define PG_decr_after		3
#define PG_active		    4
#define PG_inactive_dirty	5
#define PG_slab			    6
#define PG_swap_cache		7
#define PG_skip			    8
#define PG_inactive_clean	9
#define PG_highmem		    10
/* bits 19-29 unused */
#define PG_arch_1		    30
#define PG_reserved		    29

/* Make it prettier to test the above... */
#define Page_Uptodate(page)	    test_bit(PG_uptodate, &(page)->flags)
#define SetPageUptodate(page)	set_bit(PG_uptodate, &(page)->flags)
#define ClearPageUptodate(page)	clear_bit(PG_uptodate, &(page)->flags)
#define PageDirty(page)		    test_bit(PG_dirty, &(page)->flags)
#define SetPageDirty(page)	    set_bit(PG_dirty, &(page)->flags)
#define ClearPageDirty(page)	clear_bit(PG_dirty, &(page)->flags)
#define PageLocked(page)	    test_bit(PG_locked, &(page)->flags)
#define LockPage(page)		    set_bit(PG_locked, &(page)->flags)
#define TryLockPage(page)	    test_and_set_bit(PG_locked, (unsigned char *)&(page)->flags)

#if 0
#define UnlockPage(page)                                    \
    do                                                      \
    {                                                       \
        smp_mb__before_clear_bit();                         \
        if (!test_and_clear_bit(PG_locked, &(page)->flags)) \
            BUG();                                          \
        smp_mb__after_clear_bit();                          \
        if (waitqueue_active(&page->wait))                  \
            wake_up(&page->wait);                           \
    } while (0)
#else
#define UnlockPage(page)
#endif

#define PageError(page)		test_bit(PG_error, &(page)->flags)
#define SetPageError(page)	set_bit(PG_error, &(page)->flags)
#define ClearPageError(page)	clear_bit(PG_error, &(page)->flags)
#define PageReferenced(page)	test_bit(PG_referenced, &(page)->flags)
#define SetPageReferenced(page)	set_bit(PG_referenced, &(page)->flags)
#define ClearPageReferenced(page)	clear_bit(PG_referenced, &(page)->flags)
#define PageTestandClearReferenced(page)	test_and_clear_bit(PG_referenced, &(page)->flags)
#define PageDecrAfter(page)	test_bit(PG_decr_after, &(page)->flags)
#define SetPageDecrAfter(page)	set_bit(PG_decr_after, &(page)->flags)
#define PageTestandClearDecrAfter(page)	test_and_clear_bit(PG_decr_after, &(page)->flags)
#define PageSlab(page)		test_bit(PG_slab, &(page)->flags)
#define PageSwapCache(page)	test_bit(PG_swap_cache, &(page)->flags)
#define PageReserved(page)	test_bit(PG_reserved, &(page)->flags)

#define PageSetSlab(page)	set_bit(PG_slab, &(page)->flags)
#define PageSetSwapCache(page)	set_bit(PG_swap_cache, &(page)->flags)

#define PageTestandSetSwapCache(page)	test_and_set_bit(PG_swap_cache, &(page)->flags)

#define PageClearSlab(page)		clear_bit(PG_slab, &(page)->flags)
#define PageClearSwapCache(page)	clear_bit(PG_swap_cache, &(page)->flags)

#define PageTestandClearSwapCache(page)	test_and_clear_bit(PG_swap_cache, &(page)->flags)

#define PageActive(page)	test_bit(PG_active, &(page)->flags)
#define SetPageActive(page)	set_bit(PG_active, &(page)->flags)
#define ClearPageActive(page)	clear_bit(PG_active, &(page)->flags)

#define PageInactiveDirty(page)	test_bit(PG_inactive_dirty, &(page)->flags)
#define SetPageInactiveDirty(page)	set_bit(PG_inactive_dirty, &(page)->flags)
#define ClearPageInactiveDirty(page)	clear_bit(PG_inactive_dirty, &(page)->flags)

#define PageInactiveClean(page)	test_bit(PG_inactive_clean, &(page)->flags)
#define SetPageInactiveClean(page)	set_bit(PG_inactive_clean, &(page)->flags)
#define ClearPageInactiveClean(page)	clear_bit(PG_inactive_clean, &(page)->flags)

#ifdef CONFIG_HIGHMEM
#define PageHighMem(page)		test_bit(PG_highmem, &(page)->flags)
#else
#define PageHighMem(page)		0 /* needed to optimize away at compile time */
#endif

#define SetPageReserved(page)		set_bit(PG_reserved, &(page)->flags)
#define ClearPageReserved(page)		clear_bit(PG_reserved, &(page)->flags)

#define get_page(p)		atomic_inc(&(p)->count)
#define put_page(p)		__free_page(p)
#define put_page_testzero(p) 	atomic_dec_and_test(&(p)->count)
#define page_count(p)		atomic_read(&(p)->count)
#define set_page_count(p,v) 	atomic_set(&(p)->count, v)

/*
 * GFP bitmasks..
 */
#define __GFP_WAIT	0x01
#define __GFP_HIGH	0x02
#define __GFP_IO	0x04
#define __GFP_DMA	0x08
#ifdef CONFIG_HIGHMEM
#define __GFP_HIGHMEM	0x10
#else
#define __GFP_HIGHMEM	0x0 /* noop */
#endif


#define GFP_BUFFER	    (__GFP_HIGH | __GFP_WAIT)
#define GFP_ATOMIC	    (__GFP_HIGH)
#define GFP_USER	    (__GFP_WAIT | __GFP_IO)
#define GFP_HIGHUSER	(__GFP_WAIT | __GFP_IO   | __GFP_HIGHMEM)
#define GFP_KERNEL	    (__GFP_HIGH | __GFP_WAIT | __GFP_IO)
#define GFP_NFS		    (__GFP_HIGH | __GFP_WAIT | __GFP_IO)
#define GFP_KSWAPD	    (__GFP_IO)

/* Flag - indicates that the buffer will be suitable for DMA.  Ignored on some
   platforms, used as appropriate on others */

#define GFP_DMA		__GFP_DMA

/* Flag - indicates that the buffer can be taken from high memory which is not
   permanently mapped by the kernel */

#define GFP_HIGHMEM	__GFP_HIGHMEM
#define NOPAGE_OOM	((struct page *) (-1))

#define alloc_page(gfp_mask) alloc_pages(gfp_mask, 0)

extern struct mm_struct init_mm;

void paging_init(void);
void mem_init(unsigned long START, unsigned long END);
extern struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr);
extern struct vm_area_struct *find_vma_prev(struct mm_struct *mm, unsigned long addr, struct vm_area_struct **pprev);
extern int make_pages_present(unsigned long addr, unsigned long end);
extern void zap_page_range(struct mm_struct *mm, unsigned long address, unsigned long size);
extern int copy_page_range(struct mm_struct *dst, struct mm_struct *src, struct vm_area_struct *vma);
extern void show_mm(struct mm_struct *mm);
extern void exit_mmap(struct mm_struct *mm);
extern unsigned long do_brk(unsigned long addr, unsigned long len);
#endif

