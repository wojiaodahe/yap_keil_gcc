
#include "config.h"

const unsigned long SYSTEM_TOTAL_MEMORY_START	= 0x30000000;
const unsigned long SYSTEM_TOTAL_MEMORY_SIZE    = 0x4000000;

const unsigned long IRQ_MODE_STACK				= 0x33f00000;
const unsigned long SYS_MODE_STACK				= 0x33e00000;
const unsigned long SVC_MODE_STACK				= 0x33d00000;
const unsigned long FIQ_MODE_STACK				= 0x33c00000;
unsigned long KERNEL_SIZE                       = DEFAULT_KERNEL_SIZE;

unsigned long MMU_TLB_BASE					= TLB_BASE;



/*
|--------------------------------------------------------------------------------------------------|
|                     |                            |            |          |               |       |     
| kernel(code + data) |          free_pages        | page_table | TLB_BASE |   mem bit_map | stack |                                                          
|                     |                            |            |          |               |       |
|--------------------------------------------------------------------------------------- ----------|
*/

unsigned long get_phy_memory_size(void)
{
    return SYSTEM_TOTAL_MEMORY_SIZE;
}

unsigned long get_page_table_offset(void)
{
    return RESERVED_AREA_START;
}

static struct reserved_area RA[] = 
{
    [0] = {PHY_MEM_START, DEFAULT_KERNEL_SIZE},
    [1] = {RESERVED_AREA_START, RESERVED_AREA_SIZE},
};

unsigned long get_reserved_area(struct reserved_area **r)
{
    *r = &RA[0];

    return 2;
}
