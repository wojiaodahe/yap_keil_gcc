
#ifndef __PGALLOC_H__
#define __PGALLOC_H__

#include "page.h"
#include "mm.h"

#define pmd_free_kernel pmd_free
#define pmd_free(pmd) \
    do                \
    {                 \
    } while (0)

#define pmd_alloc_kernel pmd_alloc

#endif
