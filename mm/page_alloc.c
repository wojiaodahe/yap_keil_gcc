#include "arm/memory.h"
#include "swapctl.h"
#include "common.h"
#include "atomic.h"
#include "printk.h"
#include "mmzone.h"
#include "config.h"
#include "swap.h"
#include "page.h"
#include "lib.h"
#include "mm.h"

#define memlist_init(x) INIT_LIST_HEAD(x)
#define memlist_add_head list_add
#define memlist_add_tail list_add_tail
#define memlist_del list_del
#define memlist_entry list_entry
#define memlist_next(x) ((x)->next)
#define memlist_prev(x) ((x)->prev)

#define BAD_RANGE(zone, x) (((zone) != (x)->zone) || (((x)-mem_map) < (zone)->offset) || (((x)-mem_map) >= (zone)->offset + (zone)->size))

unsigned int memory_pressure;


int nr_swap_pages;
int nr_active_pages;
int nr_inactive_dirty_pages;
pg_data_t *pgdat_list;

static char *zone_names[MAX_NR_ZONES] = { "DMA", "Normal", "HighMem" };
static int zone_balance_ratio[MAX_NR_ZONES] = { 32, 128, 128, };
static int zone_balance_min[MAX_NR_ZONES] = { 10 , 10, 10, };
static int zone_balance_max[MAX_NR_ZONES] = { 255 , 255, 255, };

struct list_head active_list;
struct list_head inactive_dirty_list;


#define MARK_USED(index, order, area) \
	change_bit((index) >> (1+(order)), (area)->map)

static struct page *expand(zone_t *zone, struct page *page, unsigned long index, int low, int high, free_area_t *area)
{
    unsigned long size = 1 << high;

    while (high > low)
    {
        if (BAD_RANGE(zone, page))
            BUG();

        area--;
        high--;
        size >>= 1;
        memlist_add_head(&(page)->list, &(area)->free_list);
        MARK_USED(index, high, area);
        index += size;
        page += size;
    }
    if (BAD_RANGE(zone, page))
        BUG();

    return page;
}

static struct page *rmqueue(zone_t *zone, unsigned long order)
{
    free_area_t *area;
    unsigned long curr_order;
    struct list_head *head, *curr;
    //unsigned long flags;
    struct page *page;
    unsigned int index;

    area = zone->free_area + order;
    curr_order = order;

    //spin_lock_irqsave(&zone->lock, flags);
    do
    {
        head = &area->free_list;
        curr = memlist_next(head);

        if (curr != head)
        {
            page = memlist_entry(curr, struct page, list);
            if (BAD_RANGE(zone, page))
                BUG();

            memlist_del(curr);
            index = (page - mem_map) - zone->offset;
            MARK_USED(index, curr_order, area);
            zone->free_pages -= 1 << order;

            page = expand(zone, page, index, order, curr_order, area);
            //spin_unlock_irqrestore(&zone->lock, flags);
            set_page_count(page, 1);
            if (BAD_RANGE(zone, page))
                BUG();

            return page;
        }

        curr_order++;
        area++;
    } while (curr_order < MAX_ORDER);
    //spin_unlock_irqrestore(&zone->lock, flags);

    return NULL;
}

struct page *__alloc_pages(zonelist_t *zonelist, unsigned long order)
{
    zone_t *z;
    zone_t **zone;
    //unsigned int gfp_mask;
    struct page *page;

    //gfp_mask = zonelist->gfp_mask;

    memory_pressure++;

//try_again:

    zone = zonelist->zones;

    for (;;)
    {
        z = *(zone++);
        if (!z)
            break;
        if (!z->size)
            BUG();

        if (z->free_pages >= z->pages_low)
        {
            page = rmqueue(z, order);
            if (page)
                return page;
        }
    }

    return NULL;
}

struct page* alloc_pages(int gpf_mask, unsigned long order)
{
    return __alloc_pages(contig_page_data.node_zonelists + gpf_mask, order);
}

unsigned long __get_free_pages(int gfp_mask, unsigned long order)
{
    void *p;
	struct page * page;

	page = alloc_pages(gfp_mask, order);
	if (!page)
		return 0;
    
    p = page_address(page);
    memset(p, 0, (1 << order) * 4096);

	return (unsigned long)p;
}


static void __free_pages_ok(struct page *page, unsigned long order)
{
    unsigned long index, page_idx, mask, flags;
    free_area_t *area;
    struct page *base;
    zone_t *zone;

    if (page->buffers)
        BUG();
    if (page->mapping)
        BUG();
    if (!VALID_PAGE(page))
        BUG();
    if (PageSwapCache(page))
        BUG();
    if (PageLocked(page))
        BUG();
    if (PageDecrAfter(page))
        BUG();
    if (PageActive(page))
        BUG();
    if (PageInactiveDirty(page))
        BUG();
    if (PageInactiveClean(page))
        BUG();

    page->flags &= ~((1 << PG_referenced) | (1 << PG_dirty));
    page->age = PAGE_AGE_START;

    zone = page->zone;

    mask = (~0UL) << order;
    base = mem_map + zone->offset;
    page_idx = page - base;
    if (page_idx & ~mask)
        BUG();
    index = page_idx >> (1 + order);

    area = zone->free_area + order;

    spin_lock_irqsave(&zone->lock, flags);

    zone->free_pages -= mask;

    while (mask + (1 << (MAX_ORDER - 1)))
    {
        struct page *buddy1, *buddy2;

        if (area >= zone->free_area + MAX_ORDER)
            BUG();
        if (!test_and_change_bit(index, area->map))
            /*
			 * the buddy page is still allocated.
			 */
            break;
        /*
		 * Move the buddy up one level.
		 */
        buddy1 = base + (page_idx ^ -mask);
        buddy2 = base + page_idx;
        if (BAD_RANGE(zone, buddy1))
            BUG();
        if (BAD_RANGE(zone, buddy2))
            BUG();

        memlist_del(&buddy1->list);
        mask <<= 1;
        area++;
        index >>= 1;
        page_idx &= mask;
    }
    memlist_add_head(&(base + page_idx)->list, &area->free_list);

    spin_unlock_irqrestore(&zone->lock, flags);

    /*
	 * We don't want to protect this variable from race conditions
	 * since it's nothing important, but we do want to make sure
	 * it never gets negative.
	 */
   // if (memory_pressure > NR_CPUS)
        memory_pressure--;
}

void __free_pages(struct page *page, unsigned long order)
{
    if (!PageReserved(page) && put_page_testzero(page))
        __free_pages_ok(page, order);
}

void free_pages(unsigned long addr, unsigned long order)
{
    struct page *fpage;

    fpage = virt_to_page(addr);
    if (VALID_PAGE(fpage))
        __free_pages(fpage, order);
}


/*
 * Show free area list (used inside shift_scroll-lock stuff)
 * We also calculate the percentage fragmentation. We do this by counting the
 * memory on each free list with the exception of the first item on the list.
 */
void show_free_areas_core(pg_data_t *pgdat)
{
    unsigned long order;
    unsigned type;

    struct list_head *head, *curr;
    zone_t *zone;
    unsigned long nr, total, flags;

#if 0
	printk("Free pages:      %6dkB (%6dkB HighMem)\n",
		nr_free_pages() << (PAGE_SHIFT-10),
		nr_free_highpages() << (PAGE_SHIFT-10));

	printk("( Active: %d, inactive_dirty: %d, inactive_clean: %d, free: %d (%d %d %d) )\n",
		nr_active_pages,
		nr_inactive_dirty_pages,
		nr_inactive_clean_pages(),
		nr_free_pages(),
		freepages.min,
		freepages.low,
		freepages.high);
#endif
    for (type = 0; type < MAX_NR_ZONES; type++)
    {
        zone = pgdat->node_zones + type;
        if (!zone)
            continue;

        total = 0;
        if (zone->size)
        {
            spin_lock_irqsave(&zone->lock, flags);
            for (order = 0; order < MAX_ORDER; order++)
            {
                head = &(zone->free_area + order)->free_list;
                curr = head;
                nr = 0;
                for (;;)
                {
                    curr = memlist_next(curr);
                    if (curr == head)
                        break;
                    nr++;
                }
                total += nr * (1 << order);
                printk("%d*%dkB ", nr, (PAGE_SIZE >> 10) << order);
            }
            spin_unlock_irqrestore(&zone->lock, flags);
        }
        printk("= %dkB)\n", total * (PAGE_SIZE >> 10));
    }
    
    //printk("total pages: %d\n", total);


#ifdef SWAP_CACHE_INFO
    show_swap_cache_info();
#endif
}

void show_free_areas(void)
{
    show_free_areas_core(pgdat_list);
}

/*
 * Builds allocation fallback zone lists.
 */
static inline void build_zonelists(pg_data_t *pgdat)
{
    int i, j, k;

    for (i = 0; i < NR_GFPINDEX; i++)
    {
        zonelist_t *zonelist;
        zone_t *zone;

        zonelist = pgdat->node_zonelists + i;
        memset(zonelist, 0, sizeof(*zonelist));

        zonelist->gfp_mask = i;
        j = 0;
        k = ZONE_NORMAL;
        if (i & __GFP_HIGHMEM)
            k = ZONE_HIGHMEM;
        if (i & __GFP_DMA)
            k = ZONE_DMA;

        switch (k)
        {
        default:
            BUG();
        /*
			 * fallthrough:
			 */
        case ZONE_HIGHMEM:
            zone = pgdat->node_zones + ZONE_HIGHMEM;
            if (zone->size)
            {
#ifndef CONFIG_HIGHMEM
                BUG();
#endif
                zonelist->zones[j++] = zone;
            }
        case ZONE_NORMAL:
            zone = pgdat->node_zones + ZONE_NORMAL;
            if (zone->size)
                zonelist->zones[j++] = zone;
        case ZONE_DMA:
            zone = pgdat->node_zones + ZONE_DMA;
            if (zone->size)
                zonelist->zones[j++] = zone;
        }
        zonelist->zones[j++] = NULL;
    }
}

void *alloc_bootmem_node(pg_data_t *pgdat, unsigned long bitmap_size)
{
    static unsigned long offset = 0;
    char *p;

    p = (char *)(get_page_table_offset() + offset);

    memset(p, 0, bitmap_size);

    offset += bitmap_size;

    return p;
}

#define LONG_ALIGN(x) (((x) + (sizeof(long)) - 1) & ~((sizeof(long)) - 1))

void free_area_init_core(int nid, pg_data_t *pgdat, struct page **gmap,
                         unsigned long *zones_size, unsigned long zone_start_paddr,
                         unsigned long *zholes_size, struct page *lmem_map)
{
    struct page* page;
    struct page *p;
    unsigned long i, j;
    unsigned long map_size;
    unsigned long totalpages, offset, realtotalpages;
    unsigned int cumulative = 0;

    totalpages = 0;
    for (i = 0; i < MAX_NR_ZONES; i++)
    {
        unsigned long size = zones_size[i];
        totalpages += size;
    }
    realtotalpages = totalpages;

    if (zholes_size)
    {
        for (i = 0; i < MAX_NR_ZONES; i++)
            realtotalpages -= zholes_size[i];
    }

    //printk("On node %d totalpages: %d\n", nid, realtotalpages);

    memlist_init(&active_list);
    memlist_init(&inactive_dirty_list);

    /*
	 * Some architectures (with lots of mem and discontinous memory
	 * maps) have to search for a good mem_map area:
	 * For discontigmem, the conceptual mem map array starts from 
	 * PAGE_OFFSET, we need to align the actual array onto a mem map 
	 * boundary, so that MAP_NR works.
	 */
    map_size = (totalpages + 1) * sizeof(struct page);
    if (lmem_map == (struct page *)0)
    {
        lmem_map = (struct page *)alloc_bootmem_node(pgdat, map_size);
        lmem_map = (struct page *)(PAGE_OFFSET + MAP_ALIGN((unsigned long)lmem_map - PAGE_OFFSET));
    }

    *gmap = pgdat->node_mem_map = lmem_map;
    printk("mem_map: %x size: %x\n", mem_map, map_size);
    pgdat->node_size = totalpages;
    pgdat->node_start_paddr = zone_start_paddr;
    pgdat->node_start_mapnr = (lmem_map - mem_map);

    /*
	 * Initially all pages are reserved - free ones are freed
	 * up by free_all_bootmem() once the early boot process is
	 * done.
	 */
    for (p = lmem_map; p < lmem_map + totalpages; p++)
    {
        set_page_count(p, 0);
        SetPageReserved(p);
        init_waitqueue_head(&p->wait);
        memlist_init(&p->list);
    }

    offset = lmem_map - mem_map;
    for (j = 0; j < MAX_NR_ZONES; j++)
    {
        zone_t *zone = pgdat->node_zones + j;
        unsigned long mask;
        unsigned long size, realsize;

        realsize = size = zones_size[j];
        if (zholes_size)
            realsize -= zholes_size[j];

        //printk("zone(%d): %d pages.\n", j, size);
        zone->size = size;
        zone->name = zone_names[j];
        zone->lock = SPIN_LOCK_UNLOCKED;
        zone->zone_pgdat = pgdat;
        zone->free_pages = 0;
        zone->inactive_clean_pages = 0;
        zone->inactive_dirty_pages = 0;
        memlist_init(&zone->inactive_clean_list);
        if (!size)
            continue;

        zone->offset = offset;
        cumulative += size;
        mask = (realsize / zone_balance_ratio[j]);
        if (mask < zone_balance_min[j])
            mask = zone_balance_min[j];
        else if (mask > zone_balance_max[j])
            mask = zone_balance_max[j];
        zone->pages_min = mask;
        zone->pages_low = mask * 2;
        zone->pages_high = mask * 3;
        /*
		 * Add these free targets to the global free target;
		 * we have to be SURE that freepages.high is higher
		 * than SUM [zone->pages_min] for all zones, otherwise
		 * we may have bad bad problems.
		 *
		 * This means we cannot make the freepages array writable
		 * in /proc, but have to add a separate extra_free_target
		 * for people who require it to catch load spikes in eg.
		 * gigabit ethernet routing...
		 */
        freepages.min += mask;
        freepages.low += mask * 2;
        freepages.high += mask * 3;
        zone->zone_mem_map = mem_map + offset;
        zone->zone_start_mapnr = offset;
        zone->zone_start_paddr = zone_start_paddr;

        for (i = 0; i < size; i++)
        {
            page = mem_map + offset + i;
            if (i == 0)
                printk("%s page: %x\n", __func__, page);
            page->zone = zone;
            if (j != ZONE_HIGHMEM)
            {
                page->virtual = (void *)__va(zone_start_paddr);
                //printk("%p %x\n", page, page->virtual);
                zone_start_paddr += PAGE_SIZE;
            }
        }
        printk("%s page: %x\n", __func__, page);

        offset += size;
        mask = -1;
        for (i = 0; i < MAX_ORDER; i++)
        {
            unsigned long bitmap_size;

            memlist_init(&zone->free_area[i].free_list);
            mask += mask;
            size = (size + ~mask) & mask;
            bitmap_size = size >> i;
            bitmap_size = (bitmap_size + 7) >> 3;
            bitmap_size = LONG_ALIGN(bitmap_size);
            zone->free_area[i].map = (unsigned char *)alloc_bootmem_node(pgdat, bitmap_size);
            memset(zone->free_area[i].map, 0, bitmap_size);
            printk("order: %d map addr: %x size: %x\n", i, zone->free_area[i].map, bitmap_size);
        }
    }

    build_zonelists(pgdat);

    pgdat_list = pgdat;
}
