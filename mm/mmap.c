
#include "page.h"
#include "sched.h"
#include "mm.h"
#include "common.h"
#include "error.h"
#include "arm/pgalloc.h"
#include "arm/pgtable.h"

struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr)
{
    struct vm_area_struct *vma = NULL;

    if (mm)
    {
        vma = mm->mmap_cache;

        if (!(vma && vma->vm_end > addr && vma->vm_start <= addr))
        {
            if (!mm->mmap_avl)
            {
                vma = mm->mmap;
                while (vma && vma->vm_end <= addr)
                    vma = vma->vm_next;
            }
            else
            {
            }

            if (vma)
                mm->mmap_cache = vma;
        }
    }

    return vma;
}

struct vm_area_struct *find_vma_prev(struct mm_struct *mm, unsigned long addr, struct vm_area_struct **pprev)
{
    struct vm_area_struct *vma = NULL;
    struct vm_area_struct *prev = NULL;

    if (mm)
    {
        if (!mm->mmap_avl)
        {
            vma = mm->mmap;
            while (vma && vma->vm_end < addr)
            {
                prev = vma;
                vma = vma->vm_next;
            }

            *pprev = prev;
            return vma;
        }
    }

    *pprev = NULL;
    return NULL;
}

struct vm_area_struct *find_vma_intersection(struct mm_struct *mm, unsigned long start_addr, unsigned long end_addr)
{
    struct vm_area_struct *vma = find_vma(mm, start_addr);

    if (vma && end_addr <= vma->vm_start)
        vma = NULL;
    return vma;
}

void __insert_vm_struct(struct mm_struct *mm, struct vm_area_struct *vmp)
{
    struct vm_area_struct **pprev;

#if 0
    struct file *file;
#endif

    if (!mm->mmap_avl)
    {
        pprev = &mm->mmap;
        while (*pprev && (*pprev)->vm_start <= vmp->vm_start)
            pprev = &(*pprev)->vm_next;
    }
    else
    {

    }

    vmp->vm_next = *pprev;
    *pprev = vmp;

    mm->map_count++;
#if 0
	if (mm->map_count >= AVL_MIN_MAP_COUNT && !mm->mmap_avl)
		build_mmap_avl(mm);

    file = vmp->vm_file;
    if (file)
    {
        struct inode *inode = file->f_dentry->d_inode;
        struct address_space *mapping = inode->i_mapping;
        struct vm_area_struct **head;

        if (vmp->vm_flags & VM_DENYWRITE)
            atomic_dec(&inode->i_writecount);

        head = &mapping->i_mmap;
        if (vmp->vm_flags & VM_SHARED)
            head = &mapping->i_mmap_shared;

        /* insert vmp into inode's share list */
        if ((vmp->vm_next_share = *head) != NULL)
            (*head)->vm_pprev_share = &vmp->vm_next_share;
        *head = vmp;
        vmp->vm_pprev_share = head;
    }
#endif
}

void lock_vma_mappings(struct vm_area_struct *vma)
{
}

void unlock_vma_mappings(struct vm_area_struct *vma)
{
}

void insert_vm_struct(struct mm_struct *mm, struct vm_area_struct *vmp)
{
    //lock_vma_mappings(vmp);
    //spin_lock(&current->mm->page_table_lock);
    __insert_vm_struct(mm, vmp);
    //spin_unlock(&current->mm->page_table_lock);
    //unlock_vma_mappings(vmp);
}

/* Remove one vm structure from the inode's i_mapping address space. */
static inline void __remove_shared_vm_struct(struct vm_area_struct *vma)
{
#if FILE
    struct file *file = vma->vm_file;

    if (file)
    {
        struct inode *inode = file->f_dentry->d_inode;
        if (vma->vm_flags & VM_DENYWRITE)
            atomic_inc(&inode->i_writecount);
        if (vma->vm_next_share)
            vma->vm_next_share->vm_pprev_share = vma->vm_pprev_share;
        *vma->vm_pprev_share = vma->vm_next_share;
    }
#endif
}

static inline void remove_shared_vm_struct(struct vm_area_struct *vma)
{
	lock_vma_mappings(vma);
	__remove_shared_vm_struct(vma);
	unlock_vma_mappings(vma);
}

/*
 * Note: this doesn't free the actual pages themselves. That
 * has been handled earlier when unmapping all the memory regions.
 */
static  void free_one_pmd(pmd_t * dir)
{
	pte_t * pte;

	if (pmd_none(*dir))
		return;
	if (pmd_bad(*dir)) {
		pmd_ERROR(*dir);
		pmd_clear(dir);
		return;
	}
	pte = pte_offset(dir, 0);
	pmd_clear(dir);
	pte_free(pte);
}

static  void free_one_pgd(pgd_t * dir)
{
	int j;
	pmd_t * pmd;

	if (pgd_none(*dir))
		return;
	if (pgd_bad(*dir)) {
		pgd_ERROR(*dir);
		pgd_clear(dir);
		return;
	}
	pmd = pmd_offset(dir, 0);
	pgd_clear(dir);
	for (j = 0; j < PTRS_PER_PMD ; j++)
		free_one_pmd(pmd+j);
	pmd_free(pmd);
}

void clear_page_tables(struct mm_struct *mm, unsigned long first, int nr)
{
	pgd_t * page_dir = mm->pgd;

	page_dir += first;
	do {
		free_one_pgd(page_dir);
		page_dir++;
	} while (--nr);

	/* keep the page table cache within bounds */
	//check_pgt_cache();
}

void free_pgtables(struct mm_struct *mm, struct vm_area_struct *prev, unsigned long start, unsigned long end)
{
    unsigned long first = start & PGDIR_MASK;
    unsigned long last = end + PGDIR_SIZE - 1;
    unsigned long start_index, end_index;
    struct vm_area_struct *next;

    if (!prev)
    {
        prev = mm->mmap;
        if (!prev)
            goto no_mmaps;
        if (prev->vm_end > start)
        {
            if (last > prev->vm_start)
                last = prev->vm_start;
            goto no_mmaps;
        }
    }
    for (;;)
    {
        next = prev->vm_next;

        if (next)
        {
            if (next->vm_start < start)
            {
                prev = next;
                continue;
            }
            if (last > next->vm_start)
                last = next->vm_start;
        }
        if (prev->vm_end > first)
            first = prev->vm_end + PGDIR_SIZE - 1;
        break;
    }
no_mmaps:
    /*
	 * If the PGD bits are not consecutive in the virtual address, the
	 * old method of shifting the VA >> by PGDIR_SHIFT doesn't work.
	 */
    start_index = pgd_index(first);
    end_index = pgd_index(last);
    if (end_index > start_index)
    {
        clear_page_tables(mm, start_index, end_index - start_index);
        flush_tlb_pgtables(mm, first & PGDIR_MASK, last & PGDIR_MASK);
    }
}

int vm_enough_memory(long pages)
{
    return 1;
}

/* Normal function to fix up a mapping
 * This function is the default for when an area has no specific
 * function.  This may be used as part of a more specific routine.
 * This function works out what part of an area is affected and
 * adjusts the mapping information.  Since the actual page
 * manipulation is done in do_mmap(), none need be done here,
 * though it would probably be more appropriate.
 *
 * By the time this function is called, the area struct has been
 * removed from the process mapping list, so it needs to be
 * reinserted if necessary.
 *
 * The 4 main cases are:
 *    Unmapping the whole area
 *    Unmapping from the start of the segment to a point in it
 *    Unmapping from an intermediate point to the end
 *    Unmapping between to intermediate points, making a hole.
 *
 * Case 4 involves the creation of 2 new areas, for each side of
 * the hole.  If possible, we reuse the existing area rather than
 * allocate a new one, and the return indicates whether the old
 * area was reused.
 */
static struct vm_area_struct *unmap_fixup(struct mm_struct *mm, struct vm_area_struct *area, unsigned long addr, unsigned long len, struct vm_area_struct *extra)
{
    struct vm_area_struct *mpnt;
    unsigned long end = addr + len;

    area->vm_mm->total_vm -= len >> PAGE_SHIFT;
    if (area->vm_flags & VM_LOCKED)
        area->vm_mm->locked_vm -= len >> PAGE_SHIFT;

    /* Unmapping the whole area. */
    if (addr == area->vm_start && end == area->vm_end)
    {
        if (area->vm_ops && area->vm_ops->close)
            area->vm_ops->close(area);
#if FILE
        if (area->vm_file)
            fput(area->vm_file);
#endif
        kmem_cache_free(vm_area_cachep, area);
        return extra;
    }

    /* Work out to one of the ends. */
    if (end == area->vm_end)
    {
        area->vm_end = addr;
        lock_vma_mappings(area);
        spin_lock(&mm->page_table_lock);
    }
    else if (addr == area->vm_start)
    {
        area->vm_pgoff += (end - area->vm_start) >> PAGE_SHIFT;
        area->vm_start = end;
        lock_vma_mappings(area);
        spin_lock(&mm->page_table_lock);
    }
    else
    {
        /* Unmapping a hole: area->vm_start < addr <= end < area->vm_end */
        /* Add end mapping -- leave beginning for below */
        mpnt = extra;
        extra = NULL;

        mpnt->vm_mm = area->vm_mm;
        mpnt->vm_start = end;
        mpnt->vm_end = area->vm_end;
        mpnt->vm_page_prot = area->vm_page_prot;
        mpnt->vm_flags = area->vm_flags;
        mpnt->vm_raend = 0;
        mpnt->vm_ops = area->vm_ops;
        mpnt->vm_pgoff = area->vm_pgoff + ((end - area->vm_start) >> PAGE_SHIFT);
        mpnt->vm_private_data = area->vm_private_data;
#if FILE
        mpnt->vm_file = area->vm_file;
        if (mpnt->vm_file)
            get_file(mpnt->vm_file);
#endif
        if (mpnt->vm_ops && mpnt->vm_ops->open)
            mpnt->vm_ops->open(mpnt);
        area->vm_end = addr; /* Truncate area */

        /* Because mpnt->vm_file == area->vm_file this locks
		 * things correctly.
		 */
        lock_vma_mappings(area);
        spin_lock(&mm->page_table_lock);
        __insert_vm_struct(mm, mpnt);
    }

    __insert_vm_struct(mm, area);
    spin_unlock(&mm->page_table_lock);
    unlock_vma_mappings(area);
    return extra;
}

/* Munmap is split into 2 main parts -- this part which finds
 * what needs doing, and the areas themselves, which do the
 * work.  This now handles partial unmappings.
 * Jeremy Fitzhardine <jeremy@sw.oz.au>
 */
int do_munmap(struct mm_struct *mm, unsigned long addr, unsigned int len)
{
    struct vm_area_struct *mpnt, *prev, **npp, *free, *extra;

    //if ((addr & ~PAGE_MASK) || addr > TASK_SIZE || len > TASK_SIZE - addr)
    //return -EINVAL;

    if ((len = PAGE_ALIGN(len)) == 0)
        return -EINVAL;

    /* Check if this memory area is ok - put it on the temporary
	 * list if so..  The checks here are pretty simple --
	 * every area affected in some way (by any overlap) is put
	 * on the list.  If nothing is put on, nothing is affected.
	 */
    mpnt = find_vma_prev(mm, addr, &prev);
    if (!mpnt)
        return 0;
    /* we have  addr < mpnt->vm_end  */

    if (mpnt->vm_start >= addr + len)
        return 0;

    /* If we'll make "hole", check the vm areas limit */
    if ((mpnt->vm_start < addr && mpnt->vm_end > addr + len) && mm->map_count >= MAX_MAP_COUNT)
        return -ENOMEM;

    /*
	 * We may need one additional vma to fix up the mappings ... 
	 * and this is the last chance for an easy error exit.
	 */
    extra = kmem_cache_alloc(vm_area_cachep, SLAB_KERNEL);
    if (!extra)
        return -ENOMEM;

    npp = (prev ? &prev->vm_next : &mm->mmap);
    free = NULL;
    spin_lock(&mm->page_table_lock);
    for (; mpnt && mpnt->vm_start < addr + len; mpnt = *npp)
    {
        *npp = mpnt->vm_next;
        mpnt->vm_next = free;
        free = mpnt;

        //if (mm->mmap_avl)
        //   avl_remove(mpnt, &mm->mmap_avl);
    }
    mm->mmap_cache = NULL; /* Kill the cache. */
    spin_unlock(&mm->page_table_lock);

    /* Ok - we have the memory areas we should free on the 'free' list,
	 * so release them, and unmap the page range..
	 * If the one of the segments is only being partially unmapped,
	 * it will put new vm_area_struct(s) into the address space.
	 * In that case we have to be careful with VM_DENYWRITE.
	 */
    while ((mpnt = free) != NULL)
    {
        unsigned long st, end, size;
#if FILE
        struct file *file = NULL;
#endif

        free = free->vm_next;

        st = addr < mpnt->vm_start ? mpnt->vm_start : addr;
        end = addr + len;
        end = end > mpnt->vm_end ? mpnt->vm_end : end;
        size = end - st;
#if FILE
        if (mpnt->vm_flags & VM_DENYWRITE &&
            (st != mpnt->vm_start || end != mpnt->vm_end) &&
            (file = mpnt->vm_file) != NULL)
        {
            atomic_dec(&file->f_dentry->d_inode->i_writecount);
        }
#endif
        remove_shared_vm_struct(mpnt);
        mm->map_count--;

        //flush_cache_range(mm, st, end);
        zap_page_range(mm, st, size);
        //flush_tlb_range(mm, st, end);

        /*
		 * Fix the mapping, and free the old area if it wasn't reused.
		 */
        extra = unmap_fixup(mm, mpnt, st, size, extra);
#if FILE
        if (file)
            atomic_inc(&file->f_dentry->d_inode->i_writecount);
#endif
    }

    /* Release the extra vma struct if it wasn't used */
    if (extra)
        kmem_cache_free(vm_area_cachep, extra);

    free_pgtables(mm, prev, addr, addr + len);

    return 0;
}

long sys_munmap(unsigned long addr, unsigned int len)
{
    int ret;
    struct mm_struct *mm = current->mm;

    //down(&mm->mmap_sem);
    ret = do_munmap(mm, addr, len);
    //up(&mm->mmap_sem);
    return ret;
}

pgprot_t protection_map[16] = {
    {__P000}, {__P001}, {__P010}, {__P011}, {__P100}, {__P101}, {__P110}, {__P111},
    {__S000}, {__S001}, {__S010}, {__S011}, {__S100}, {__S101}, {__S110}, {__S111}
};

/*
 *  this is really a simplified "do_mmap".  it only handles
 *  anonymous maps.  eventually we may be able to do some
 *  brk-specific accounting here.
 */
unsigned long do_brk(unsigned long addr, unsigned long len)
{
    struct mm_struct *mm = current->mm;
    struct vm_area_struct *vma;
    unsigned long flags, retval;

    len = PAGE_ALIGN(len);
    if (!len)
        return addr;

#if 0
    /*
	 * mlock MCL_FUTURE?
	 */
    if (mm->def_flags & VM_LOCKED)
    {
        unsigned long locked = mm->locked_vm << PAGE_SHIFT;
        locked += len;
        if (locked > current->rlim[RLIMIT_MEMLOCK].rlim_cur)
            return -EAGAIN;
    }
#endif
    /*
	 * Clear old maps.  this also does some error checking for us
	 */
    retval = do_munmap(mm, addr, len);
    if (retval != 0)
        return retval;

#if 0
    /* Check against address space limits *after* clearing old maps... */
    if ((mm->total_vm << PAGE_SHIFT) + len > current->rlim[RLIMIT_AS].rlim_cur)
        return -ENOMEM;
#endif

    if (mm->map_count > MAX_MAP_COUNT)
        return -ENOMEM;

    if (!vm_enough_memory(len >> PAGE_SHIFT))
        return -ENOMEM;

    flags = VM_DATA_DEFAULT_FLAGS | mm->def_flags;

    flags |= VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC | VM_LOCKED;

    /* Can we just expand an old anonymous mapping? */
    if (addr)
    {
        vma = find_vma(mm, addr - 1);
#if FILE
        if (vma && vma->vm_end == addr && !vma->vm_file && vma->vm_flags == flags)
        {
            vma->vm_end = addr + len;
            goto out;
        }
#else
        if (vma && vma->vm_end == addr && vma->vm_flags == flags)
        {
            vma->vm_end = addr + len;
            goto out;
        }
#endif
    }

    /*
	 * create a vma struct for an anonymous mapping
	 */
    vma = kmem_cache_alloc(vm_area_cachep, SLAB_KERNEL);
    if (!vma)
        return -ENOMEM;

    vma->vm_mm = mm;
    vma->vm_start = addr;
    vma->vm_end = addr + len;
    vma->vm_flags = flags;
    vma->vm_page_prot = protection_map[flags & 0x0f];
    vma->vm_ops = NULL;
    vma->vm_pgoff = 0;
#if FILE
    vma->vm_file = NULL;
#endif
    vma->vm_private_data = NULL;

    insert_vm_struct(mm, vma);

out:
    mm->total_vm += len >> PAGE_SHIFT;
    if (flags & VM_LOCKED)
    {
        mm->locked_vm += len >> PAGE_SHIFT;
        make_pages_present(addr, addr + len);
    }
    return addr;
}

unsigned long sys_brk(unsigned long brk)
{
    //unsigned long rlim;
    unsigned long retval;
    unsigned long newbrk, oldbrk;
    struct mm_struct *mm = current->mm;

    //down(&mm->mmap_sem);

    if (brk < mm->end_code)
        goto out;
    newbrk = PAGE_ALIGN(brk);
    oldbrk = PAGE_ALIGN(mm->brk);
    if (oldbrk == newbrk)
        goto set_brk;

    /* Always allow shrinking brk. */
    if (brk <= mm->brk)
    {
        if (!do_munmap(mm, newbrk, oldbrk - newbrk))
            goto set_brk;
        goto out;
    }
#if 0
    /* Check against rlimit.. */
    rlim = current->rlim[RLIMIT_DATA].rlim_cur;
    if (rlim < RLIM_INFINITY && brk - mm->start_data > rlim)
        goto out;
#endif
    /* Check against existing mmap mappings. */
    if (find_vma_intersection(mm, oldbrk, newbrk + PAGE_SIZE))
        goto out;
    
    /* Check if we have enough memory.. */
    if (!vm_enough_memory((newbrk - oldbrk) >> PAGE_SHIFT))
        goto out;
   
    /* Ok, looks good - let it rip. */
    if (do_brk(oldbrk, newbrk - oldbrk) != oldbrk)
        goto out;
set_brk:
    mm->brk = brk;
out:
    retval = mm->brk;
    //	up(&mm->mmap_sem);
    return retval;
}

/* Release all mmaps. */
void exit_mmap(struct mm_struct *mm)
{
    struct vm_area_struct *mpnt;

    spin_lock(&mm->page_table_lock);
    mpnt = mm->mmap;
    mm->mmap = mm->mmap_avl = mm->mmap_cache = NULL;
    spin_unlock(&mm->page_table_lock);
    mm->rss = 0;
    mm->total_vm = 0;
    mm->locked_vm = 0;
    while (mpnt)
    {
        struct vm_area_struct *next = mpnt->vm_next;
        unsigned long start = mpnt->vm_start;
        unsigned long end = mpnt->vm_end;
        unsigned long size = end - start;

        if (mpnt->vm_ops)
        {
            if (mpnt->vm_ops->close)
                mpnt->vm_ops->close(mpnt);
        }
        mm->map_count--;
        remove_shared_vm_struct(mpnt);
        //flush_cache_range(mm, start, end); //TODO fix me here
        zap_page_range(mm, start, size);
#if FILE
        if (mpnt->vm_file)
            fput(mpnt->vm_file);
#endif
        kmem_cache_free(vm_area_cachep, mpnt);
        mpnt = next;
    }

    /* This is just debugging */
    if (mm->map_count)
        printf("exit_mmap: map count is %d\n", mm->map_count);

    clear_page_tables(mm, FIRST_USER_PGD_NR, USER_PTRS_PER_PGD);
}

void show_pte(pte_t *pte)
{
    int i;
    //unsigned long *p;

    if (!pte)
        return;

    //p = (unsigned long *)pte->pte;
    for (i = 0; i < 1024; i++)
    {
        if (pte_val(*pte))
        {
            printf("i: %d pte: %p pte->pte: %lx\n", i, pte, pte->pte);
        }
        pte++; 
    }
}

void show_pmd(pmd_t *pmd)
{
    int i;
    pte_t *pte;

    pte = pte_alloc(pmd, 0);
    
    for (i = 0; i < 1024; i++)
    {
        if (pte_val(*pte))
        {
            printf("pte%d: %p pte->pte: %lx\n", i, pte, pte->pte);
        }
        pte++;
    }
}

void show_pgd(pgd_t *pgd)
{
    int i;
    pmd_t *pmd;

    pmd = pmd_alloc(pgd, 0);
    
    for (i = 0; i < 1024; i++)
    {
        if (pmd_val(*pmd))
        {
            printf("pmd%d: %p pmd->pmd: %lx\n", i, pmd, pmd->pmd);
            show_pmd(pmd);
        }
        pmd++;
    }
}

void show_mm(struct mm_struct *mm)
{
    struct vm_area_struct *vma = NULL;

    if (mm)
    {
        vma = mm->mmap;

        printf("\nmm: %p\n", mm);
        printf("mm->mmap: %p\n", mm->mmap);
        printf("mm->def_flags: %lx\n", mm->def_flags);
        printf("mm->pgd: %p\n", mm->pgd);
        show_pgd(mm->pgd);

        while (vma)
        {
            printf("vma->start: %lx\n", vma->vm_start);
            printf("vma->end: %lx\n", vma->vm_end);
            vma = vma->vm_next;
        }
    }
}
