#include "mmzone.h"
#include "page.h"
#include "mm.h"
#include "memory.h"

#include "config.h"

/*
 * empty_bad_page is the page that is used for page faults when
 * linux is out-of-memory. Older versions of linux just did a
 * do_exit(), but using this instead means there is less risk
 * for a process dying in kernel mode, possibly leaving a inode
 * unused etc..
 *
 * empty_bad_pte_table is the accompanying page-table: it is
 * initialized to point to BAD_PAGE entries.
 *
 * empty_zero_page is a special page that is used for
 * zero-initialized data and COW.
 */
struct page *empty_zero_page;
struct page *empty_bad_page;
pte_t *empty_bad_pte_table;

unsigned long free_all_bootmem_core(pg_data_t *pgdat);

struct bootmem_data bdata;
pg_data_t contig_page_data;

int numnodes = 1;
unsigned long totalram_pages = 0;

void mem_init(unsigned long START, unsigned long END)
{
    int i;
    int node;
    unsigned long ra_count;
    struct reserved_area *ra;

    bdata.node_boot_start = START;
    bdata.node_low_pfn = (END >> PAGE_SHIFT);
    max_mapnr = bdata.node_low_pfn;

    contig_page_data.bdata = &bdata;

    bdata.node_bootmem_map = (void *)alloc_bootmem_node(&contig_page_data, 0x10000);

    ra_count = get_reserved_area(&ra);
    for (i = 0; i < ra_count; i++)
    {
        reserve_bootmem_core(&bdata, ra->start, ra->size);
        ra++;
    }

    for (node = 0; node < numnodes; node++)
        totalram_pages = free_all_bootmem_core(NODE_DATA(node));

    empty_zero_page = alloc_page(GFP_KERNEL);
}

void free_area_init(unsigned long *zones_size)
{
    free_area_init_core(0, &contig_page_data, &mem_map, zones_size, 0, 0, 0);
}

void paging_init(void)
{

    unsigned long zones_size[MAX_NR_ZONES] = {0, 0, 0};

    zones_size[ZONE_NORMAL] = get_phy_memory_size() >> PAGE_SHIFT;

    free_area_init(zones_size);
}