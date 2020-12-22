#include "mm.h"
#include "lib.h"
#include "printk.h"
#include "page.h"
#include "error.h"
#include "sched.h"
#include "pagemap.h"
#include "arm/pgtable.h"


mem_map_t *mem_map;

unsigned long max_mapnr;
unsigned long num_physpages;


unsigned long PHY_MEM_START;
unsigned long PHY_MEM_END;
unsigned long PHY_ALLOC_START;

#define MEM_SIZE            0x10000000
#define MEM_MAP_SIZE        0x200000
#define MEM_BIT_MAP_SIZE    0x300000

int phy_mem_init(void)
{
    void *ptr;

    ptr = malloc(MEM_SIZE + PAGE_SIZE * 4);

    PHY_MEM_START = (((unsigned long)(ptr)) & ~(PAGE_SIZE - 1)) + PAGE_SIZE;
    PHY_MEM_END = PHY_MEM_START + MEM_SIZE;
    PHY_ALLOC_START = PHY_MEM_START + MEM_MAP_SIZE + MEM_BIT_MAP_SIZE;

    printk("PHY_MEM_START:      %lx\n", PHY_MEM_START);
    printk("PHY_MEM_END:        %lx\n", PHY_MEM_END);
    printk("PHY_ALLOC_START:    %lx\n", PHY_ALLOC_START);

    return 0;
}

/*
 * Add a page to the dirty page list.
 */
void __set_page_dirty(struct page *page)
{
#if FILE
    struct address_space *mapping = page->mapping;

    spin_lock(&pagecache_lock);
    list_del(&page->list);
    list_add(&page->list, &mapping->dirty_pages);
    spin_unlock(&pagecache_lock);

    mark_inode_dirty_pages(mapping->host);
#endif
}

void set_page_dirty(struct page *page)
{
    if (!test_and_set_bit(PG_dirty, (unsigned char *)&page->flags))
        __set_page_dirty(page);
}

/* 
 * Perform a free_page(), also freeing any swap cache associated with
 * this page if it is the last user of the page. Can not do a lock_page,
 * as we are holding the page_table_lock spinlock.
 */
void free_page_and_swap_cache(struct page *page)
{
#if SWAP
    /* 
	 * If we are the only user, then try to free up the swap cache. 
	 */
    if (PageSwapCache(page) && !TryLockPage(page))
    {
        if (!is_page_shared(page))
        {
            delete_from_swap_cache_nolock(page);
        }
        UnlockPage(page); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    }
#endif
    page_cache_release(page);
}

#define PTE_TABLE_MASK ((PTRS_PER_PTE - 1) * sizeof(pte_t))
#define PMD_TABLE_MASK ((PTRS_PER_PMD - 1) * sizeof(pmd_t))

/*
 * copy one vm_area from one task to the other. Assumes the page tables
 * already present in the new task to be cleared in the whole range
 * covered by this vma.
 *
 * 08Jan98 Merged into one routine from several inline routines to reduce
 *         variable count and make things faster. -jj
 */
int copy_page_range(struct mm_struct *dst, struct mm_struct *src, struct vm_area_struct *vma)
{
    pgd_t *src_pgd, *dst_pgd;
    unsigned long address = vma->vm_start;
    unsigned long end = vma->vm_end;
    unsigned long cow = (vma->vm_flags & (VM_SHARED | VM_MAYWRITE)) == VM_MAYWRITE;

    src_pgd = pgd_offset(src, address) - 1;
    dst_pgd = pgd_offset(dst, address) - 1;

    for (;;)
    {
        pmd_t *src_pmd, *dst_pmd;

        src_pgd++;
        dst_pgd++;

        /* copy_pmd_range */

        if (pgd_none(*src_pgd))
            goto skip_copy_pmd_range;
        if (pgd_bad(*src_pgd))
        {
            pgd_ERROR(*src_pgd);
            pgd_clear(src_pgd);
skip_copy_pmd_range:
            address = (address + PGDIR_SIZE) & PGDIR_MASK;
            if (!address || (address >= end))
                goto out;
            continue;
        }
        if (pgd_none(*dst_pgd))
        {
            if (!pmd_alloc(dst_pgd, 0))
                goto nomem;
        }

        src_pmd = pmd_offset(src_pgd, address);
        dst_pmd = pmd_offset(dst_pgd, address);

        do
        {
            pte_t *src_pte, *dst_pte;

            /* copy_pte_range */

            if (pmd_none(*src_pmd))
                goto skip_copy_pte_range;
            if (pmd_bad(*src_pmd))
            {
                pmd_ERROR(*src_pmd);
                pmd_clear(src_pmd);
skip_copy_pte_range:
                address = (address + PMD_SIZE) & PMD_MASK;
                if (address >= end)
                    goto out;
                goto cont_copy_pmd_range;
            }
            if (pmd_none(*dst_pmd))
            {
                if (!pte_alloc(dst_pmd, 0))
                    goto nomem;
            }

            src_pte = pte_offset(src_pmd, address);
            dst_pte = pte_offset(dst_pmd, address);

            do
            {
                pte_t pte = *src_pte;
                struct page *ptepage;

                /* copy_one_pte */

                if (pte_none(pte))
                    goto cont_copy_pte_range_noset;
                if (!pte_present(pte))
                {
#if SWAP
                    swap_duplicate(pte_to_swp_entry(pte));
#endif
                    goto cont_copy_pte_range;
                }
                ptepage = pte_page(pte);
                if ((!VALID_PAGE(ptepage)) ||
                    PageReserved(ptepage))
                    goto cont_copy_pte_range;

                /* If it's a COW mapping, write protect it both in the parent and the child */
                if (cow)
                {
                    ptep_set_wrprotect(src_pte);
                    pte = *src_pte;
                }

                /* If it's a shared mapping, mark it clean in the child */
                if (vma->vm_flags & VM_SHARED)
                    pte = pte_mkclean(pte);
                pte = pte_mkold(pte);
                get_page(ptepage);

cont_copy_pte_range:
                set_pte(dst_pte, pte);
cont_copy_pte_range_noset:
                address += PAGE_SIZE;
                if (address >= end)
                    goto out;
                src_pte++;
                dst_pte++;
            } while ((unsigned long)src_pte & PTE_TABLE_MASK);

cont_copy_pmd_range:
            src_pmd++;
            dst_pmd++;
        } while ((unsigned long)src_pmd & PMD_TABLE_MASK);
    }
out:
    return 0;

nomem:
    return -ENOMEM;
}

/*
 * Return indicates whether a page was freed so caller can adjust rss
 */
static int free_pte(pte_t pte)
{
    if (pte_present(pte))
    {
        struct page *page = pte_page(pte);
        if ((!VALID_PAGE(page)) || PageReserved(page))
            return 0;
        /* 
		 * free_page() used to be able to clear swap cache
		 * entries.  We may now have to do it manually.  
		 */
        if (pte_dirty(pte) && page->mapping)
            set_page_dirty(page);
        free_page_and_swap_cache(page);
        return 1;
    }
#if SWAP
    swap_free(pte_to_swp_entry(pte));
#endif
    return 0;
}

static int zap_pte_range(struct mm_struct *mm, pmd_t *pmd, unsigned long address, unsigned long size)
{
    pte_t *pte;
    int freed;

    if (pmd_none(*pmd))
        return 0;
    if (pmd_bad(*pmd))
    {
        pmd_ERROR(*pmd);
        pmd_clear(pmd);
        return 0;
    }
    pte = pte_offset(pmd, address);
    address &= ~PMD_MASK;
    if (address + size > PMD_SIZE)
        size = PMD_SIZE - address;
    size >>= PAGE_SHIFT;
    freed = 0;
    for (;;)
    {
        pte_t page;
        if (!size)
            break;
        page = ptep_get_and_clear(pte);
        pte++;
        size--;
        if (pte_none(page))
            continue;
        freed += free_pte(page);
    }
    return freed;
}

static int zap_pmd_range(struct mm_struct *mm, pgd_t *dir, unsigned long address, unsigned long size)
{
    pmd_t *pmd;
    unsigned long end;
    int freed;

    if (pgd_none(*dir))
        return 0;
    if (pgd_bad(*dir))
    {
        pgd_ERROR(*dir);
        pgd_clear(dir);
        return 0;
    }
    pmd = pmd_offset(dir, address);
    address &= ~PGDIR_MASK;
    end = address + size;
    if (end > PGDIR_SIZE)
        end = PGDIR_SIZE;
    freed = 0;
    do
    {
        freed += zap_pte_range(mm, pmd, address, end - address);
        address = (address + PMD_SIZE) & PMD_MASK;
        pmd++;
    } while (address < end);
    return freed;
}

/*
 * remove user pages in a given range.
 */
void zap_page_range(struct mm_struct *mm, unsigned long address, unsigned long size)
{
    pgd_t *dir;
    unsigned long end = address + size;
    int freed = 0;

    dir = pgd_offset(mm, address);

    /*
	 * This is a long-lived spinlock. That's fine.
	 * There's no contention, because the page table
	 * lock only protects against kswapd anyway, and
	 * even if kswapd happened to be looking at this
	 * process we _want_ it to get stuck.
	 */
    if (address >= end)
        BUG();
    spin_lock(&mm->page_table_lock);
    do
    {
        freed += zap_pmd_range(mm, dir, address, end - address);
        address = (address + PGDIR_SIZE) & PGDIR_MASK;
        dir++;
    } while (address && (address < end));
    spin_unlock(&mm->page_table_lock);
    /*
	 * Update rss for the mm_struct (not necessarily current->mm)
	 * Notice that rss is an unsigned long.
	 */
    if (mm->rss > freed)
        mm->rss -= freed;
    else
        mm->rss = 0;
}

void establish_pte(struct vm_area_struct * vma, unsigned long address, pte_t *page_table, pte_t entry)
{
	set_pte(page_table, entry);
	//flush_tlb_page(vma, address);
	//update_mmu_cache(vma, address, entry);
}


/*
 * Work out if there are any other processes sharing this page, ignoring
 * any page reference coming from the swap cache, or from outstanding
 * swap IO on this page.  (The page cache _does_ count as another valid
 * reference to the page, however.)
 */
static int is_page_shared(struct page *page)
{
#if SWAP
    unsigned int count;
    if (PageReserved(page))
        return 1;
    count = page_count(page);
    if (PageSwapCache(page))
        count += swap_count(page) - 2 - !!page->buffers;
    return count > 1;
#else
    return 0;
#endif
}

void copy_cow_page(struct page * from, struct page * to, unsigned long address)
{

}

void break_cow(struct vm_area_struct * vma, struct page *	old_page, struct page * new_page, unsigned long address, 
		pte_t *page_table)
{
	copy_cow_page(old_page,new_page,address);
    //flush_page_to_ram(new_page);
	//flush_cache_page(vma, address);
	establish_pte(vma, address, page_table, pte_mkwrite(pte_mkdirty(mk_pte(new_page, vma->vm_page_prot))));
}

/*
 * This routine handles present pages, when users try to write
 * to a shared page. It is done by copying the page to a new address
 * and decrementing the shared-page counter for the old page.
 *
 * Goto-purists beware: the only reason for goto's here is that it results
 * in better assembly code.. The "default" path will see no jumps at all.
 *
 * Note that this routine assumes that the protection checks have been
 * done by the caller (the low-level page fault routine in most cases).
 * Thus we can safely just mark it writable once we've done any necessary
 * COW.
 *
 * We also mark the page dirty at this point even though the page will
 * change only once the write actually happens. This avoids a few races,
 * and potentially makes it more efficient.
 *
 * We enter with the page table read-lock held, and need to exit without
 * it.
 */
static int do_wp_page(struct mm_struct *mm, struct vm_area_struct *vma, unsigned long address, pte_t *page_table, pte_t pte)
{
    struct page *old_page, *new_page;

    old_page = pte_page(pte);
    if (!VALID_PAGE(old_page))
        goto bad_wp_page;

    /*
	 * We can avoid the copy if:
	 * - we're the only user (count == 1)
	 * - the only other user is the swap cache,
	 *   and the only swap cache user is itself,
	 *   in which case we can just continue to
	 *   use the same swap cache (it will be
	 *   marked dirty).
	 */
    switch (page_count(old_page))
    {
    case 2:
        /*
		 * Lock the page so that no one can look it up from
		 * the swap cache, grab a reference and start using it.
		 * Can not do lock_page, holding page_table_lock.
		 */
        if (!PageSwapCache(old_page) || TryLockPage(old_page))
            break;
        if (is_page_shared(old_page))
        {
            UnlockPage(old_page);
            break;
        }
        UnlockPage(old_page);
        /* FallThrough */
    case 1:
        //flush_cache_page(vma, address);
        establish_pte(vma, address, page_table, pte_mkyoung(pte_mkdirty(pte_mkwrite(pte))));
        spin_unlock(&mm->page_table_lock);
        return 1; /* Minor fault */
    }

    /*
	 * Ok, we need to copy. Oh, well..
	 */
    spin_unlock(&mm->page_table_lock);
    new_page = (struct page *)page_cache_alloc();
    if (!new_page)
        return -1;
    spin_lock(&mm->page_table_lock);

    /*
	 * Re-check the pte - we dropped the lock
	 */
    if (pte_same(*page_table, pte))
    {
        if (PageReserved(old_page))
            ++mm->rss;
        break_cow(vma, old_page, new_page, address, page_table);

        /* Free the old page.. */
        new_page = old_page;
    }
    spin_unlock(&mm->page_table_lock);
    page_cache_release(new_page);
    return 1; /* Minor fault */

bad_wp_page:
    spin_unlock(&mm->page_table_lock);
    printk("do_wp_page: bogus page at address %08lx (page 0x%lx)\n", address, (unsigned long)old_page);
    return -1;
}

#if SWAP
static int do_swap_page(struct mm_struct *mm,
                        struct vm_area_struct *vma, unsigned long address,
                        pte_t *page_table, swp_entry_t entry, int write_access)
{
    struct page *page = lookup_swap_cache(entry);
    pte_t pte;

    if (!page)
    {
        lock_kernel();
        swapin_readahead(entry);
        page = read_swap_cache(entry);
        unlock_kernel();
        if (!page)
            return -1;

        flush_page_to_ram(page);
        flush_icache_page(vma, page);
    }

    mm->rss++;

    pte = mk_pte(page, vma->vm_page_prot);

    /*
	 * Freeze the "shared"ness of the page, ie page_count + swap_count.
	 * Must lock page before transferring our swap count to already
	 * obtained page count.
	 */
    lock_page(page);
    swap_free(entry);
    if (write_access && !is_page_shared(page))
        pte = pte_mkwrite(pte_mkdirty(pte));
    UnlockPage(page);

    set_pte(page_table, pte);
    /* No need to invalidate - it was non-present before */
    update_mmu_cache(vma, address, pte);
    return 1; /* Minor fault */
}
#endif

/*
 * This only needs the MM semaphore
 */
static int do_anonymous_page(struct mm_struct *mm, struct vm_area_struct *vma, pte_t *page_table, int write_access, unsigned long addr)
{
    struct page *page = NULL;
    pte_t entry = pte_wrprotect(mk_pte(ZERO_PAGE(addr), vma->vm_page_prot));
    if (write_access)
    {
        page = (struct page *)alloc_page(GFP_HIGHUSER);
        if (!page)
            return -1;
        //clear_user_highpage(page, addr);
        entry = pte_mkwrite(pte_mkdirty(mk_pte(page, vma->vm_page_prot)));
        mm->rss++;
        //flush_page_to_ram(page);
    }
    set_pte(page_table, entry);
    /* No need to invalidate - it was non-present before */
    //update_mmu_cache(vma, addr, entry);
    return 1; /* Minor fault */
}

/*
 * do_no_page() tries to create a new page mapping. It aggressively
 * tries to share with existing pages, but makes a separate copy if
 * the "write_access" parameter is true in order to avoid the next
 * page fault.
 *
 * As this is called only for pages that do not currently exist, we
 * do not need to flush old virtual caches or the TLB.
 *
 * This is called with the MM semaphore held.
 */
static int do_no_page(struct mm_struct *mm, struct vm_area_struct *vma,
                      unsigned long address, int write_access, pte_t *page_table)
{
    struct page *new_page;
    pte_t entry;

    if (!vma->vm_ops || !vma->vm_ops->nopage)
        return do_anonymous_page(mm, vma, page_table, write_access, address);

    /*
	 * The third argument is "no_share", which tells the low-level code
	 * to copy, not share the page even if sharing is possible.  It's
	 * essentially an early COW detection.
	 */
    new_page = vma->vm_ops->nopage(vma, address & PAGE_MASK, (vma->vm_flags & VM_SHARED) ? 0 : write_access);
    if (new_page == NULL) /* no page was available -- SIGBUS */
        return 0;
    if (new_page == NOPAGE_OOM)
        return -1;
    ++mm->rss;
    /*
	 * This silly early PAGE_DIRTY setting removes a race
	 * due to the bad i386 page protection. But it's valid
	 * for other architectures too.
	 *
	 * Note that if write_access is true, we either now have
	 * an exclusive copy of the page, or this is a shared mapping,
	 * so we can make it writable and dirty to avoid having to
	 * handle that later.
	 */
    //flush_page_to_ram(new_page);
    //flush_icache_page(vma, new_page);
    entry = mk_pte(new_page, vma->vm_page_prot);
    if (write_access)
    {
        entry = pte_mkwrite(pte_mkdirty(entry));
    }
    else if (page_count(new_page) > 1 && !(vma->vm_flags & VM_SHARED))
        entry = pte_wrprotect(entry);
    set_pte(page_table, entry);
    /* no need to invalidate: a not-present page shouldn't be cached */
    //update_mmu_cache(vma, address, entry);
    return 2; /* Major fault */
}

/*
 * These routines also need to handle stuff like marking pages dirty
 * and/or accessed for architectures that don't do it in hardware (most
 * RISC architectures).  The early dirtying is also good on the i386.
 *
 * There is also a hook called "update_mmu_cache()" that architectures
 * with external mmu caches can use to update those (ie the Sparc or
 * PowerPC hashed page tables that act as extended TLBs).
 *
 * Note the "page_table_lock". It is to protect against kswapd removing
 * pages from under us. Note that kswapd only ever _removes_ pages, never
 * adds them. As such, once we have noticed that the page is not present,
 * we can drop the lock early.
 *
 * The adding of pages is protected by the MM semaphore (which we hold),
 * so we don't need to worry about a page being suddenly been added into
 * our VM.
 */
static inline int handle_pte_fault(struct mm_struct *mm, struct vm_area_struct *vma, unsigned long address, int write_access, pte_t *pte)
{
    pte_t entry;

    /*
	 * We need the page table lock to synchronize with kswapd
	 * and the SMP-safe atomic PTE updates.
	 */
    spin_lock(&mm->page_table_lock);
    entry = *pte;
    if (!pte_present(entry))
    {
        /*
		 * If it truly wasn't present, we know that kswapd
		 * and the PTE updates will not touch it later. So
		 * drop the lock.
		 */
        spin_unlock(&mm->page_table_lock);
        if (pte_none(entry))
            return do_no_page(mm, vma, address, write_access, pte);
#if SWAP
        return do_swap_page(mm, vma, address, pte, pte_to_swp_entry(entry), write_access);
#endif
    }

    if (write_access)
    {
        if (!pte_write(entry))
            return do_wp_page(mm, vma, address, pte, entry);

        entry = pte_mkdirty(entry);
    }
    entry = pte_mkyoung(entry);
    establish_pte(vma, address, pte, entry);
    spin_unlock(&mm->page_table_lock);
    return 1;
}

/*
 * By the time we get here, we already hold the mm semaphore
 */
int handle_mm_fault(struct mm_struct *mm, struct vm_area_struct *vma, unsigned long address, int write_access)
{
    int ret = -1;
    pgd_t *pgd;
    pmd_t *pmd;

    pgd = pgd_offset(mm, address);
    pmd = pmd_alloc(pgd, address);

    if (pmd)
    {
        pte_t *pte = pte_alloc(pmd, address);
        if (pte)
            ret = handle_pte_fault(mm, vma, address, write_access, pte);
    }
    return ret;
}

/*
 * Simplistic page force-in..
 */
int make_pages_present(unsigned long addr, unsigned long end)
{
    int write;
    struct mm_struct *mm = current->mm;
    struct vm_area_struct *vma;

    vma = find_vma(mm, addr);
    write = (vma->vm_flags & VM_WRITE) != 0;
    if (addr >= end)
        BUG();
    do
    {
        if (handle_mm_fault(mm, vma, addr, write) < 0)
            return -1;
        addr += PAGE_SIZE;
    } while (addr < end);

    return 0;
}
