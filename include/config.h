#ifndef __KERNEL_CONFIG_H__
#define __KERNEL_CONFIG_H__

#define PHY_MEM_START   				0x30000000
#define VIRTUAL_MEM_ADDR				0x30000000
#define MEM_SIZE					    0x4000000
#define PHY_MEM_END       				(PHY_MEM_START + MEM_SIZE)

#define PHYSICAL_IO_ADDR				0x48000000
#define VIRTUAL_IO_ADDR					0x48000000
#define IO_MAP_SIZE						0x18000000

#define VIRTUAL_VECTOR_ADDR				0x0
#define PHYSICAL_VECTOR_ADDR			0x30000000

#define TLB_BASE                		0x33a00000              

#define IRQ_STACK				        0x33f00000
#define FIQ_STACK				        0x33e00000
#define SVC_STACK				        0x33d00000
#define SYS_STACK				        0x33c00000

#define DEFAULT_KERNEL_SIZE             0x500000
#define PAGE_TABLE_SIZE                 0x200000
#define MEM_BIT_MAP_SIZE                0x300000

#define RESERVED_AREA_START            (0x33500000)
#define RESERVED_AREA_SIZE             (PHY_MEM_END - RESERVED_AREA_START)

#define TASK_STACK_SIZE					4096

#define MAX_TASK_NUM                    128

#define MAX_UDP_LINK_NUM				64
#define MAX_TCP_LINK_NUM				64


struct reserved_area
{
    unsigned long start;
    unsigned long size;
};

extern unsigned long get_page_table_offset(void);
extern unsigned long get_phy_memory_size(void);

#endif 


