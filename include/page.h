#ifndef __PAGE_H__
#define __PAGE_H__

#include "bug.h"
#include "bitops.h"
#include "list.h"
#include "atomic.h"
#include "wait.h"
#include "mmzone.h"

#include "arm/memory.h"
//#include "arm/pgtable.h"
//#include "arm/pgtable.h"

#define PAGE_SHIFT      (12)
#define PAGE_SIZE       (1UL << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE - 1))


typedef struct pte
{
    unsigned long pte;
} pte_t;

typedef struct pmd
{
    unsigned long pmd;
} pmd_t;

typedef struct pgd
{
    unsigned long pgd;
} pgd_t;

typedef struct pgprot
{
    unsigned long pgprot;
}pgprot_t;

#define pte_val(x)              ((x).pte)
#define pmd_val(x)              ((x).pmd)
#define pgd_val(x)              ((x).pgd)
#define pgprot_val(x)           ((x).pgprot)
#define pte_page(x)             (mem_map + (unsigned long)((x).pte >> PAGE_SHIFT))

#define __pte(x)                ((pte_t) { (x) } )
#define __pmd(x)                ((pmd_t) { (x) } )
#define __pgd(x)                ((pgd_t) { (x) } )
#define __pgprot(x)             (x)

#define pmd_none(pmd)		(!pmd_val(pmd))
#define pmd_clear(pmdp)		set_pmd(pmdp, __pmd(0))

#define set_pte(ptep, pte) (*(ptep) = pte)
#define set_pmd(pmdp, pmd) (*(pmdp) = pmd) 
#define pmd_bad(pmd) (pmd_val(pmd) & 2)
#define pmd_offset(dir, addr)	((pmd_t *)(dir))


#define page_pte_prot(page,prot)	mk_pte(page, prot)
#define page_pte(page)		mk_pte(page, __pgprot(0))

/* to find an entry in a page-table-directory */
#define pgd_index(addr)		((addr) >> PGDIR_SHIFT)
#define __pgd_offset(addr)	pgd_index(addr)

#define pgd_offset(mm, addr) ((mm)->pgd + pgd_index(addr))

/* to find an entry in a kernel page-table-directory */
#define pgd_offset_k(addr)	pgd_offset(&init_mm, addr)

/* Find an entry in the second-level page table.. */
#define pmd_offset(dir, addr)	((pmd_t *)(dir))

/* Find an entry in the third-level page table.. */
#define __pte_offset(addr)	(((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))
#define pte_offset(dir, addr)	((pte_t *)pmd_page(*(dir)) + __pte_offset(addr))

#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & PAGE_MASK)

#define PTE_MASK        PAGE_MASK

#define PAGE_OFFSET             (0x0000000)
//#define page_address(page)      (__va(page_to_pfn(page) << PAGE_SHIFT))
#define page_address(page)      ((void *)(page_to_pfn(page) << PAGE_SHIFT))

typedef struct page
{
    struct list_head list;
    struct addres_space *mapping;
    unsigned long index;
    atomic_t count;
    unsigned long flags;
    struct list_head lru;
    unsigned long age;
    //wait_queue_head_t wait;
    struct buffer_head *buffers;
    void *virtual;
    struct zone_struct *zone;
} mem_map_t;

extern mem_map_t *mem_map;

extern unsigned long max_mapnr;
extern unsigned long num_physpages;

#define VALID_PAGE(page)    ((page - mem_map) < max_mapnr)


void free_area_init_core(int nid, pg_data_t *pgdat, struct page **gmap,
                         unsigned long *zones_size, unsigned long zone_start_paddr,
                         unsigned long *zholes_size, struct page *lmem_map);


#endif



