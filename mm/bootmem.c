#include "bootmem.h"
#include "page.h"
#include "mmzone.h"

#include "mm.h"

void reserve_bootmem_core(bootmem_data_t *bdata, unsigned long addr, unsigned long size)
{
	unsigned long i;
	/*
	 * round up, partially reserved pages are considered
	 * fully reserved.
	 */
	unsigned long sidx = (addr - bdata->node_boot_start) / PAGE_SIZE;
	unsigned long eidx = (addr + size - bdata->node_boot_start + PAGE_SIZE - 1) / PAGE_SIZE;
	unsigned long end = (addr + size + PAGE_SIZE - 1) / PAGE_SIZE;

	if (!size)
		BUG();

	if (end > bdata->node_low_pfn)
		BUG();

	for (i = sidx; i < eidx; i++)
	{
		if (test_and_set_bit(i, bdata->node_bootmem_map))
			printk("hm, page %08lx reserved twice.\n", i * PAGE_SIZE);
	}
}

unsigned long free_all_bootmem_core(pg_data_t *pgdat)
{
	struct page *page = pgdat->node_mem_map;
	bootmem_data_t *bdata = pgdat->bdata;
	unsigned long i, count, total = 0;
	unsigned long idx;

	count = 0;
	idx = bdata->node_low_pfn - (bdata->node_boot_start >> PAGE_SHIFT);
	for (i = 0; i < idx; i++, page++)
	{
		if (!test_bit(i, bdata->node_bootmem_map))
		{
			count++;
			ClearPageReserved(page);
			set_page_count(page, 1);
			__free_page(page);
		}
	}
	total += count;

#if 0
    /*
	 * Now free the allocator bitmap itself, it's not
	 * needed anymore:
	 */
	page = virt_to_page(bdata->node_bootmem_map);
	count = 0;
	for (i = 0; i < ((bdata->node_low_pfn-(bdata->node_boot_start >> PAGE_SHIFT))/8 + PAGE_SIZE-1)/PAGE_SIZE; i++,page++) {
		count++;
		ClearPageReserved(page);
		set_page_count(page, 1);
		__free_page(page);
	}
	total += count;
	bdata->node_bootmem_map = NULL;
#endif

	return total;
}