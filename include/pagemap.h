#ifndef __PAGE_MAP_H__
#define __PAGE_MAP_H__

#include "page.h"
#include "mm.h"

#define page_cache_get(x)	get_page(x)
#define page_cache_alloc()	alloc_pages(GFP_HIGHUSER, 0)
#define page_cache_free(x)	__free_page(x)
#define page_cache_release(x)	__free_page(x)

#endif
