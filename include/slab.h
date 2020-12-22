#ifndef __LINUX_SLAB_H__
#define __LINUX_SLAB_H__

#include "mm.h"
#include "list.h"
#include "spinlock.h"

#define BUFCTL_END 0xffffFFFF
#define	SLAB_LIMIT 0xffffFFFE
typedef unsigned int kmem_bufctl_t;

typedef struct slab_s
{
    struct list_head list;
    unsigned long colouroff;
    void *s_mem;
    unsigned int inuse;
    kmem_bufctl_t free;
} slab_t;

#define slab_bufctl(slabp) ((kmem_bufctl_t *) (((slab_t *)slabp) + 1))

#define CACHE_NAMELEN   20
typedef struct kmem_cache_s
{
    /* 1) each alloc & free */
    /* full, partial first, then free */
    struct list_head slabs;
    struct list_head *firstnotfull;
    unsigned int objsize;
    unsigned int flags; /* constant flags */
    unsigned int num;   /* # of objs per slab */
    spinlock_t spinlock;

#ifdef CONFIG_SMP
    unsigned int batchcount;
#endif

    /* 2) slab additions /removals */
    /* order of pgs per slab (2^n) */
    unsigned int gfporder;

    /* force GFP flags, e.g. GFP_DMA */
    unsigned int gfpflags;

    unsigned int colour;            /* cache colouring range */
    unsigned int colour_off;  /* colour offset */
    unsigned int colour_next; /* cache colouring */
    struct kmem_cache_s *slabp_cache;
    unsigned int growing;
    unsigned int dflags; /* dynamic flags */

    /* constructor func */
    void (*ctor)(void *, struct kmem_cache_s *, unsigned long);

    /* de-constructor func */
    void (*dtor)(void *, struct kmem_cache_s *, unsigned long);

    unsigned long failures;

    /* 3) cache creation/removal */
    char name[CACHE_NAMELEN];
    struct list_head next;

#ifdef CONFIG_SMP
    /* 4) per-cpu data */
    cpucache_t *cpudata[NR_CPUS];
#endif

#if STATS
    unsigned long num_active;
    unsigned long num_allocations;
    unsigned long high_mark;
    unsigned long grown;
    unsigned long reaped;
    unsigned long errors;
#ifdef CONFIG_SMP
    atomic_t allochit;
    atomic_t allocmiss;
    atomic_t freehit;
    atomic_t freemiss;
#endif
#endif
} kmem_cache_t;

/* internal c_flags */
#define	CFLGS_OFF_SLAB	0x010000UL	/* slab management in own cache */
#define	CFLGS_OPTIMIZE	0x020000UL	/* optimized slab lookup */

/* c_dflags (dynamic flags). Need to hold the spinlock to access this member */
#define	DFLGS_GROWN	0x000001UL	/* don't reap a recently grown */

#define	OFF_SLAB(x)	((x)->flags & CFLGS_OFF_SLAB)
#define	OPTIMIZE(x)	((x)->flags & CFLGS_OPTIMIZE)
#define	GROWN(x)	((x)->dlags & DFLGS_GROWN)

/* flags for kmem_cache_alloc() */
#define	SLAB_BUFFER		GFP_BUFFER
#define	SLAB_ATOMIC		GFP_ATOMIC
#define	SLAB_USER		GFP_USER
#define	SLAB_KERNEL		GFP_KERNEL
#define	SLAB_NFS		GFP_NFS
#define	SLAB_DMA		GFP_DMA

#define SLAB_LEVEL_MASK		(__GFP_WAIT|__GFP_HIGH|__GFP_IO)
#define	SLAB_NO_GROW		0x00001000UL	/* don't grow a cache */

/* flags to pass to kmem_cache_create().
 * The first 3 are only valid when the allocator as been build
 * SLAB_DEBUG_SUPPORT.
 */
#define	SLAB_DEBUG_FREE		0x00000100UL	/* Peform (expensive) checks on free */
#define	SLAB_DEBUG_INITIAL	0x00000200UL	/* Call constructor (as verifier) */
#define	SLAB_RED_ZONE		0x00000400UL	/* Red zone objs in a cache */
#define	SLAB_POISON		0x00000800UL	/* Poison objects */
#define	SLAB_NO_REAP		0x00001000UL	/* never reap from the cache */
#define	SLAB_HWCACHE_ALIGN	0x00002000UL	/* align objs on a h/w cache lines */
#define SLAB_CACHE_DMA		0x00004000UL	/* use GFP_DMA memory */

/* flags passed to a constructor func */
#define	SLAB_CTOR_CONSTRUCTOR	0x001UL		/* if not set, then deconstructor */
#define SLAB_CTOR_ATOMIC	0x002UL		/* tell constructor it can't sleep */
#define	SLAB_CTOR_VERIFY	0x004UL		/* tell constructor it's a verify call */


/* Macros for storing/retrieving the cachep and or slab from the
 * global 'mem_map'. These are used to find the slab an obj belongs to.
 * With kfree(), these are used to find the cache which an obj belongs to.
 */
#define	SET_PAGE_CACHE(pg,x)  ((pg)->list.next = (struct list_head *)(x))
#define	GET_PAGE_CACHE(pg)    ((kmem_cache_t *)(pg)->list.next)
#define	SET_PAGE_SLAB(pg,x)   ((pg)->list.prev = (struct list_head *)(x))
#define	GET_PAGE_SLAB(pg)     ((slab_t *)(pg)->list.prev)


#if STATS
#define	STATS_INC_ACTIVE(x)	((x)->num_active++)
#define	STATS_DEC_ACTIVE(x)	((x)->num_active--)
#define	STATS_INC_ALLOCED(x)	((x)->num_allocations++)
#define	STATS_INC_GROWN(x)	((x)->grown++)
#define	STATS_INC_REAPED(x)	((x)->reaped++)
#define	STATS_SET_HIGH(x)	do { if ((x)->num_active > (x)->high_mark) \
					(x)->high_mark = (x)->num_active; \
				} while (0)
#define	STATS_INC_ERR(x)	((x)->errors++)
#else
#define	STATS_INC_ACTIVE(x)	do { } while (0)
#define	STATS_DEC_ACTIVE(x)	do { } while (0)
#define	STATS_INC_ALLOCED(x)	do { } while (0)
#define	STATS_INC_GROWN(x)	do { } while (0)
#define	STATS_INC_REAPED(x)	do { } while (0)
#define	STATS_SET_HIGH(x)	do { } while (0)
#define	STATS_INC_ERR(x)	do { } while (0)
#endif

#if STATS && defined(CONFIG_SMP)
#define STATS_INC_ALLOCHIT(x)	atomic_inc(&(x)->allochit)
#define STATS_INC_ALLOCMISS(x)	atomic_inc(&(x)->allocmiss)
#define STATS_INC_FREEHIT(x)	atomic_inc(&(x)->freehit)
#define STATS_INC_FREEMISS(x)	atomic_inc(&(x)->freemiss)
#else
#define STATS_INC_ALLOCHIT(x)	do { } while (0)
#define STATS_INC_ALLOCMISS(x)	do { } while (0)
#define STATS_INC_FREEHIT(x)	do { } while (0)
#define STATS_INC_FREEMISS(x)	do { } while (0)
#endif

#define	BYTES_PER_WORD		sizeof(void *)
#define	MAX_OBJ_ORDER	5	/* 32 pages */


#define	MAX_GFP_ORDER	5	/* 32 pages */


# define CREATE_MASK	(SLAB_HWCACHE_ALIGN | SLAB_NO_REAP | SLAB_CACHE_DMA)


void *kmem_cache_alloc(kmem_cache_t *cachep, int flags);
void kmem_cache_free(kmem_cache_t *cachep, void *objp);
void kmem_freepages(kmem_cache_t *cachep, void *addr);
kmem_cache_t *kmem_cache_create(char *name, unsigned int size, unsigned int offset,
                                unsigned long flags, void (*ctor)(void *, kmem_cache_t *, unsigned long),
                                void (*dtor)(void *, kmem_cache_t *, unsigned long));

#endif
