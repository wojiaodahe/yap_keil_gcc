#include "pgtable.h"
#include "memory.h"
#include "page.h"

unsigned long pmd_page(pmd_t pmd)
{
	unsigned long val;

	val = pmd.pmd;

	return __phys_to_virt(pmd_val(pmd) &~0xfff & ~_PAGE_TABLE);
}


pte_t ptep_get_and_clear(pte_t *ptep)
{
	pte_t pte = *ptep;
	pte_clear(ptep);
	return pte;
}

pte_t pte_nocache(pte_t pte)
{
	return pte;
}
pte_t pte_wrprotect(pte_t pte)
{
	pte_val(pte) |= _PAGE_READONLY;
	return pte;
}
pte_t pte_rdprotect(pte_t pte)
{
	pte_val(pte) |= _PAGE_NOT_USER;
	return pte;
}
pte_t pte_exprotect(pte_t pte)
{
	pte_val(pte) |= _PAGE_NOT_USER;
	return pte;
}
pte_t pte_mkclean(pte_t pte)
{
	pte_val(pte) |= _PAGE_CLEAN;
	return pte;
}
pte_t pte_mkold(pte_t pte)
{
	pte_val(pte) |= _PAGE_OLD;
	return pte;
}

pte_t pte_mkwrite(pte_t pte)
{
	pte_val(pte) &= ~_PAGE_READONLY;
	return pte;
}
pte_t pte_mkread(pte_t pte)
{
	pte_val(pte) &= ~_PAGE_NOT_USER;
	return pte;
}
pte_t pte_mkexec(pte_t pte)
{
	pte_val(pte) &= ~_PAGE_NOT_USER;
	return pte;
}
pte_t pte_mkdirty(pte_t pte)
{
	pte_val(pte) &= ~_PAGE_CLEAN;
	return pte;
}
pte_t pte_mkyoung(pte_t pte)
{
	pte_val(pte) &= ~_PAGE_OLD;
	return pte;
}


 void ptep_set_wrprotect(pte_t *ptep)
{
	pte_t old_pte = *ptep;
	set_pte(ptep, pte_wrprotect(old_pte));
}