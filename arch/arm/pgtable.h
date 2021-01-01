#ifndef __ARM_PGTABLE_H__
#define __ARM_PGTABLE_H__

#include "domain.h"
#include "page.h"

/* PTE types (actually level 2 descriptor) */
#define PTE_TYPE_MASK		0x0003
#define PTE_TYPE_FAULT		0x0000
#define PTE_TYPE_LARGE		0x0001
#define PTE_TYPE_SMALL		0x0002
#define PTE_AP_READ		    0x0aa0
#define PTE_AP_WRITE		0x0550
#define PTE_CACHEABLE		0x0008
#define PTE_BUFFERABLE		0x0004


#define L_PTE_PRESENT		(1 << 0)
#define L_PTE_YOUNG		    (1 << 1)
#define L_PTE_BUFFERABLE	(1 << 2)
#define L_PTE_CACHEABLE		(1 << 3)
#define L_PTE_USER		    (1 << 4)
#define L_PTE_WRITE		    (1 << 5)
#define L_PTE_EXEC		    (1 << 6)
#define L_PTE_DIRTY		    (1 << 7)


/*
 * The following only work if pte_present() is true.
 * Undefined behaviour if not..
 */
#define pte_present(pte)		(pte_val(pte) & L_PTE_PRESENT)
#define pte_read(pte)			(pte_val(pte) & L_PTE_USER)
#define pte_write(pte)			(pte_val(pte) & L_PTE_WRITE)
#define pte_exec(pte)			(pte_val(pte) & L_PTE_EXEC)
#define pte_dirty(pte)			(pte_val(pte) & L_PTE_DIRTY)
#define pte_young(pte)			(pte_val(pte) & L_PTE_YOUNG)

extern pte_t pte_nocache(pte_t pte);
extern pte_t pte_wrprotect(pte_t pte);    
extern pte_t pte_rdprotect(pte_t pte);    
extern pte_t pte_exprotect(pte_t pte);    
extern pte_t pte_mkclean(pte_t pte);    
extern pte_t pte_mkold(pte_t pte);    

extern pte_t pte_mkwrite(pte_t pte);    
extern pte_t pte_mkread(pte_t pte);    
extern pte_t pte_mkexec(pte_t pte);    
extern pte_t pte_mkdirty(pte_t pte);    
extern pte_t pte_mkyoung(pte_t pte);    
extern void ptep_set_wrprotect(pte_t *ptep);

/*
 * entries per page directory level: they are two-level, so
 * we don't really have any PMD directory.
 */
#define PTRS_PER_PTE		256
#define PTRS_PER_PMD		1
#define PTRS_PER_PGD		4096

/* PMD types (actually level 1 descriptor) */
#define PMD_TYPE_MASK		0x0003
#define PMD_TYPE_FAULT		0x0000
#define PMD_TYPE_TABLE		0x0001
#define PMD_TYPE_SECT		0x0002
#define PMD_UPDATABLE		0x0010
#define PMD_SECT_CACHEABLE	0x0008
#define PMD_SECT_BUFFERABLE	0x0004
#define PMD_SECT_AP_WRITE	0x0400
#define PMD_SECT_AP_READ	0x0800
#define PMD_DOMAIN(x)		((x) << 5)

#define _PAGE_USER_TABLE	(PMD_TYPE_TABLE | PMD_DOMAIN(DOMAIN_USER))
#define _PAGE_KERNEL_TABLE	(PMD_TYPE_TABLE | PMD_DOMAIN(DOMAIN_KERNEL))

#define mk_user_pmd(ptep)	\
({\
	pmd_t pmd;\
	unsigned long pte_ptr = (unsigned long)ptep;\
	pmd_val(pmd) = __virt_to_phys(pte_ptr);\
    pmd;\
})

#define mk_kernel_pmd(ptep)\
({\
	pmd_t pmd;\
	unsigned long pte_ptr = (unsigned long)ptep;\
	pmd_val(pmd) = __virt_to_phys(pte_ptr);\
    pmd;\
})

#define PMD_SHIFT		    20
#define PGDIR_SHIFT		    20

#define _PAGE_TABLE         (0x01)

#define PMD_SIZE            (1UL << PMD_SHIFT)
#define PMD_MASK            (~(PMD_SIZE - 1))
#define PGDIR_SIZE          (1UL << PGDIR_SHIFT)
#define PGDIR_MASK          (~(PGDIR_SIZE - 1))

unsigned long pmd_page(pmd_t pmd);
pmd_t __mk_pmd(pte_t *ptep, unsigned long prot);
#define pte_clear(ptep)		set_pte((ptep), __pte(0))

pgd_t *pgd_alloc(void);
pmd_t *pmd_alloc(pgd_t *pgd, unsigned long address);
pte_t *pte_alloc(pmd_t *pmd, unsigned long address);
pte_t ptep_get_and_clear(pte_t *ptep);
void pte_free(pte_t *pte);

#define mk_pte(page, pgprot)\
({\
	pte_t __pte;\
	pte_val(__pte) = __pa(page_address(page)) + pgprot_val(pgprot);\
	__pte;\
})

#define pte_none(pte)		(!pte_val(pte))

#define pgd_none(pgd)		(0)
#define pgd_bad(pgd)		(0)
#define pgd_present(pgd)	(1)
#define pgd_clear(pgdp)

#define pte_same(A,B)	(pte_val(A) == pte_val(B))

extern void __pte_error(const char *file, int line, unsigned long val);
extern void __pmd_error(const char *file, int line, unsigned long val);
extern void __pgd_error(const char *file, int line, unsigned long val);

#define pte_ERROR(pte)		__pte_error(__FILE__, __LINE__, pte_val(pte))
#define pmd_ERROR(pmd)		__pmd_error(__FILE__, __LINE__, pmd_val(pmd))
#define pgd_ERROR(pgd)		__pgd_error(__FILE__, __LINE__, pgd_val(pgd))

#define flush_tlb_pgtables(mm,start,end)	do { } while (0)

#define _L_PTE_DEFAULT	L_PTE_PRESENT | L_PTE_YOUNG
#define _L_PTE_READ	L_PTE_USER | L_PTE_CACHEABLE | L_PTE_BUFFERABLE

#define _PAGE_PRESENT	0x01
#define _PAGE_READONLY	0x02
#define _PAGE_NOT_USER	0x04
#define _PAGE_OLD	0x08
#define _PAGE_CLEAN	0x10


#define PAGE_NONE       __pgprot(_L_PTE_DEFAULT)
#define PAGE_COPY       __pgprot(_L_PTE_DEFAULT | _L_PTE_READ)
#define PAGE_SHARED     __pgprot(_L_PTE_DEFAULT | _L_PTE_READ | L_PTE_WRITE)
#define PAGE_READONLY   __pgprot(_L_PTE_DEFAULT | _L_PTE_READ)
#define PAGE_KERNEL     __pgprot(_L_PTE_DEFAULT | L_PTE_CACHEABLE | L_PTE_BUFFERABLE | L_PTE_DIRTY | L_PTE_WRITE)

#define __P000  PAGE_NONE
#define __P001  PAGE_READONLY
#define __P010  PAGE_COPY
#define __P011  PAGE_COPY
#define __P100  PAGE_READONLY
#define __P101  PAGE_READONLY
#define __P110  PAGE_COPY
#define __P111  PAGE_COPY

#define __S000  PAGE_NONE
#define __S001  PAGE_READONLY
#define __S010  PAGE_SHARED
#define __S011  PAGE_SHARED
#define __S100  PAGE_READONLY
#define __S101  PAGE_READONLY
#define __S110  PAGE_SHARED
#define __S111  PAGE_SHARED

#define FIRST_USER_PGD_NR 1
#define USER_PTRS_PER_PGD ((TASK_SIZE / PGDIR_SIZE) - FIRST_USER_PGD_NR)

extern struct page *empty_zero_page;
#define ZERO_PAGE(vaddr)	(empty_zero_page)

#endif
