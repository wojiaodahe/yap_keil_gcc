
#include "lib.h"
#include "printk.h"
#include "pgtable.h"
#include "page.h"
#include "mm.h"
#include "slab.h"
#include "common.h"
#include "pgalloc.h"

extern void cpu_arm920_clean_dentry_and_drain_Wb(void);

void set_pmd(pmd_t *pmd, pmd_t entry)
{
    entry.pmd &= ~(0x1ff);

    pmd->pmd = entry.pmd | (3 << 5) | 0x01;

    cpu_arm920_clean_dentry_and_drain_Wb();
}

void set_pte(pte_t *pte_table, pte_t entry)
{
    entry.pte &= ~(0xfff);

    pte_table->pte = entry.pte | 0xff0 | (1 << 2) | (1 << 3) | 0x02;
    cpu_arm920_clean_dentry_and_drain_Wb();
}


pte_t *get_bad_pte_table(void)
{
    return NULL;
}

void __handle_bad_pmd(pmd_t *pmd)
{
    printk("%s Bad Pmd\n", __func__);
    //pmd_ERROR(*pmd);
    set_pmd(pmd, mk_user_pmd(get_bad_pte_table()));
}

pte_t *get_pte_slow(pmd_t *pmd, unsigned long offset)
{
    pte_t *pte;

    pte = (pte_t *)__get_free_pages(GFP_KERNEL, 0);
    if (pmd_none(*pmd))
    {
        if (pte)
        {
#if 0
            memzero(pte, 2 * PTRS_PER_PTE * sizeof(pte_t));
            clean_cache_area(pte, PTRS_PER_PTE * sizeof(pte_t));
            pte += PTRS_PER_PTE;
#endif
            set_pmd(pmd, mk_user_pmd(pte));
            return pte + offset;
        }
        set_pmd(pmd, mk_user_pmd(get_bad_pte_table()));
        return NULL;
    }
    free_pages((unsigned long)pte, 0);
    if (pmd_bad(*pmd))
    {
        __handle_bad_pmd(pmd);
        return NULL;
    }
    return (pte_t *)pmd_page(*pmd) + offset;
}

pte_t *get_pte_fast(void)
{
    return NULL;
}

pte_t *pte_alloc(pmd_t *pmd, unsigned long address)
{
    address = (address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1);
    if (pmd_none(*pmd))
    {
        pte_t *page = (pte_t *)get_pte_fast();

        if (!page)
            return get_pte_slow(pmd, address);
        set_pmd(pmd, mk_user_pmd(page));
        return page + address;
    }
    if (pmd_bad(*pmd))
    {
        __handle_bad_pmd(pmd);
        return NULL;
    }
    return (pte_t *)pmd_page(*pmd) + address;
}

void pte_free(pte_t *pte)
{
    free_pages((unsigned long)pte, 0);
}

pmd_t *pmd_alloc(pgd_t *pgd, unsigned long address)
{
    return (pmd_t *)pgd;
}

/*
 * need to get a 16k page for level 1
 */
pgd_t *get_pgd_slow(void)
{
    pgd_t *pgd = (pgd_t *)__get_free_pages(GFP_KERNEL, 2);
    pmd_t *new_pmd;

    if (0)
    {
        pgd_t *init = pgd_offset_k(0);
#if 0
        memzero(pgd, FIRST_KERNEL_PGD_NR * sizeof(pgd_t));
        memcpy(pgd + FIRST_KERNEL_PGD_NR, init + FIRST_KERNEL_PGD_NR,
               (PTRS_PER_PGD - FIRST_KERNEL_PGD_NR) * sizeof(pgd_t));
#endif
        /*
		 * FIXME: this should not be necessary
		 */
        //clean_cache_area(pgd, PTRS_PER_PGD * sizeof(pgd_t));

        /*
		 * On ARM, first page must always be allocated
		 */
        if (!pmd_alloc(pgd, 0))
            goto nomem;
        else
        {
            pmd_t *old_pmd = pmd_offset(init, 0);
            new_pmd = pmd_offset(pgd, 0);

            if (!pte_alloc(new_pmd, 0))
                goto nomem_pmd;
            else
            {
                pte_t *new_pte = pte_offset(new_pmd, 0);
                pte_t *old_pte = pte_offset(old_pmd, 0);

                set_pte(new_pte, *old_pte);
            }
        }
    }
    return pgd;

nomem_pmd:
    pmd_free(new_pmd);
nomem:
    free_pages((unsigned long)pgd, 2);
    return NULL;
}

pgd_t *get_pgd_fast(void)
{
    return NULL;
}

pgd_t *pgd_alloc(void)
{
    pgd_t *pgd;

    pgd = get_pgd_fast();
    if (!pgd)
        pgd = get_pgd_slow();

    return pgd;
}
