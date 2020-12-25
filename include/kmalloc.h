#ifndef __KMALLOC_H__
#define __KMALLOC_H__

#include "mm.h"

extern void kfree(void *addr);
extern void *kmalloc(unsigned int size, unsigned int flags);
extern int system_mm_init(void);
extern void system_mm_destroy(void);
#endif

