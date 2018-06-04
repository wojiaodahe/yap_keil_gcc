#ifndef __KERNEL_CONFIG_H__
#define __KERNEL_CONFIG_H__

#define PHYSICAL_MEM_ADDR				0x30000000
#define VIRTUAL_MEM_ADDR				0x30000000
#define MEM_MAP_SIZE					0x4000000

#define PHYSICAL_IO_ADDR				0x48000000
#define VIRTUAL_IO_ADDR					0x48000000
#define IO_MAP_SIZE						0x18000000

#define VIRTUAL_VECTOR_ADDR				0x0
#define PHYSICAL_VECTOR_ADDR			0x30000000

#define TLB_BASE                		0x33a00000                //TLB基址(1级页表基址)

#define USER_PROGRAM_SPACE_START		0x32000000
#define USER_PROGRAM_SPACE_SIZE			(TLB_BASE - USER_PROGRAM_SPACE_START)

#define USER_PROGRAM_SPACE_UNIT_SIZE 	(1024 * 1024)
#define MAX_USER_PROGRAM_NUM			(USER_PROGRAM_SPACE_SIZE / USER_PROGRAM_SPACE_UNIT_SIZE)

#define MAX_TASK_NUM					MAX_USER_PROGRAM_NUM
#define TASK_STACK_SIZE					4096

#define HZ								100

#endif 


