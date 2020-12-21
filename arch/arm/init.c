#include "mmzone.h"
#include "page.h"
#include "mm.h"
#include "memory.h"

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

void mem_init(unsigned long PHY_MEM_START, unsigned long PHY_MEM_END)
{
    int node;
     
    bdata.node_boot_start = PHY_MEM_START;
    bdata.node_low_pfn = (PHY_MEM_END >> PAGE_SHIFT);
    max_mapnr = bdata.node_low_pfn;

    contig_page_data.bdata = &bdata;
    
    for (node = 0; node < numnodes; node++)
        totalram_pages = free_all_bootmem_core(NODE_DATA(node));
}

void free_area_init(unsigned long *zones_size)
{
    free_area_init_core(0, &contig_page_data, &mem_map, zones_size, 0, 0, 0);
}

void paging_init(void)
{
    unsigned long zones_size[MAX_NR_ZONES] = {0, 0, 0};

    zones_size[ZONE_NORMAL] = (PHY_MEM_END - PHY_MEM_START) >> PAGE_SHIFT;

    free_area_init(zones_size);
}