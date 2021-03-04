#ifndef __BOOTMEM_H__
#define __BOOTMEM_H__


typedef struct bootmem_data {
	unsigned long node_boot_start;
	unsigned long node_low_pfn;
	void *node_bootmem_map;
	unsigned long last_offset;
	unsigned long last_pos;
} bootmem_data_t;

extern void reserve_bootmem_core(bootmem_data_t *bdata, unsigned long addr, unsigned long size);

#endif

