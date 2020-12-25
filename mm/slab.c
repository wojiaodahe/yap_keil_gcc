#include "common.h"
#include "arm/irqflags.h"
#include "arm/cache.h"
#include "bug.h"
#include "slab.h"
#include "interrupt.h"
#include "printk.h"
#include "lib.h"

void *kmem_getpages(kmem_cache_t *cachep, unsigned long flags)
{
    void *addr;

    /*
	 * If we requested dmaable memory, we will get it. Even if we
	 * did not request dmaable memory, we might get it, but that
	 * would be relatively rare and ignorable.
	 */
    flags |= cachep->gfpflags;
    addr = (void *)__get_free_pages(flags, cachep->gfporder);
    /* Assume that now we have the pages no one else can legally
	 * messes with the 'struct page's.
	 * However vm_scan() might try to test the structure to see if
	 * it is a named-page or buffer-page.  The members it tests are
	 * of no interest here.....
	 */
    return addr;
}

/* Get the memory for a slab management obj. */
static inline slab_t *kmem_cache_slabmgmt(kmem_cache_t *cachep, void *objp, int colour_off, int local_flags)
{
    slab_t *slabp;

    if (OFF_SLAB(cachep))
    {
        /* Slab management obj is off-slab. */
        slabp = kmem_cache_alloc(cachep->slabp_cache, local_flags);
        if (!slabp)
            return NULL;
    }
    else
    {
        /* FIXME: change to
			slabp = objp
		 * if you enable OPTIMIZE
		 */
        slabp = objp + colour_off;
        colour_off += L1_CACHE_ALIGN(cachep->num * sizeof(kmem_bufctl_t) + sizeof(slab_t));
    }
    slabp->inuse = 0;
    slabp->colouroff = colour_off;
    slabp->s_mem = (void *)((char *)objp + colour_off);


    return slabp;
}

void kmem_cache_init_objs(kmem_cache_t *cachep, slab_t *slabp, unsigned long ctor_flags)
{
    int i;

    for (i = 0; i < cachep->num; i++)
    {
        void *objp = slabp->s_mem + cachep->objsize * i;

        /*
		 * Constructors are not allowed to allocate memory from
		 * the same cache which they are a constructor for.
		 * Otherwise, deadlock. They must also be threaded.
		 */
        if (cachep->ctor)
            cachep->ctor(objp, cachep, ctor_flags); 

        slab_bufctl(slabp)[i] = i + 1;
    }
    slab_bufctl(slabp)[i - 1] = BUFCTL_END;
    slabp->free = 0;
}

int kmem_cache_grow(kmem_cache_t *cachep, int flags)
{
    slab_t *slabp;
    struct page *page;
    void *objp;
    unsigned int offset;
    unsigned int i, local_flags;
    unsigned long ctor_flags;
    unsigned long save_flags;

    /* Be lazy and only check for valid flags here,
 	 * keeping it out of the critical path in kmem_cache_alloc().
	 */
    if (flags & ~(SLAB_DMA | SLAB_LEVEL_MASK | SLAB_NO_GROW))
        BUG();
    if (flags & SLAB_NO_GROW)
        return 0; 

    /*
	 * The test for missing atomic flag is performed here, rather than
	 * the more obvious place, simply to reduce the critical path length
	 * in kmem_cache_alloc(). If a caller is seriously mis-behaving they
	 * will eventually be caught here (where it matters).
	 */
    //if (in_interrupt() && (flags & SLAB_LEVEL_MASK) != SLAB_ATOMIC)
    //  BUG();

    ctor_flags = SLAB_CTOR_CONSTRUCTOR;
    local_flags = (flags & SLAB_LEVEL_MASK);
    if (local_flags == SLAB_ATOMIC)
        /*
		 * Not allowed to sleep.  Need to tell a constructor about
		 * this - it might need to know...
		 */
        ctor_flags |= SLAB_CTOR_ATOMIC;

    /* About to mess with non-constant members - lock. */
    spin_lock_irqsave(&cachep->spinlock, save_flags);

    /* Get colour for the slab, and cal the next value. */
    offset = cachep->colour_next;
    cachep->colour_next++;
    if (cachep->colour_next >= cachep->colour)
        cachep->colour_next = 0;
    offset *= cachep->colour_off;
    cachep->dflags |= DFLGS_GROWN;

    cachep->growing++;
    spin_unlock_irqrestore(&cachep->spinlock, save_flags);

    /* A series of memory allocations for a new slab.
	 * Neither the cache-chain semaphore, or cache-lock, are
	 * held, but the incrementing c_growing prevents this
	 * cache from being reaped or shrunk.
	 * Note: The cache could be selected in for reaping in
	 * kmem_cache_reap(), but when the final test is made the
	 * growing value will be seen.
	 */

    /* Get mem for the objs. */
    if (!(objp = kmem_getpages(cachep, flags)))
        goto failed;

    /* Get slab management. */
    if (!(slabp = kmem_cache_slabmgmt(cachep, objp, offset, local_flags)))
        goto opps1;

    /* Nasty!!!!!! I hope this is OK. */
    i = 1 << cachep->gfporder;
    page = virt_to_page(objp);
    do
    {
        SET_PAGE_CACHE(page, cachep);
        SET_PAGE_SLAB(page, slabp);
        PageSetSlab(page);
        page++;
    } while (--i);

    kmem_cache_init_objs(cachep, slabp, ctor_flags);

    spin_lock_irqsave(&cachep->spinlock, save_flags);
    cachep->growing--;

    /* Make slab active. */
    list_add_tail(&slabp->list, &cachep->slabs);
    if (cachep->firstnotfull == &cachep->slabs)
        cachep->firstnotfull = &slabp->list;
    STATS_INC_GROWN(cachep);
    cachep->failures = 0;

    spin_unlock_irqrestore(&cachep->spinlock, save_flags);
    return 1;
opps1:
    kmem_freepages(cachep, objp);
failed:
    spin_lock_irqsave(&cachep->spinlock, save_flags);
    cachep->growing--;
    spin_unlock_irqrestore(&cachep->spinlock, save_flags);
    return 0;
}

void *kmem_cache_alloc_one_tail(kmem_cache_t *cachep, slab_t *slabp)
{
    void *objp;

    STATS_INC_ALLOCED(cachep);
    STATS_INC_ACTIVE(cachep);
    STATS_SET_HIGH(cachep);

    /* get obj pointer */
    slabp->inuse++;
    objp = slabp->s_mem + slabp->free * cachep->objsize;
    slabp->free = slab_bufctl(slabp)[slabp->free];

    if (slabp->free == BUFCTL_END)
        /* slab now full: move to next slab for next alloc */
        cachep->firstnotfull = slabp->list.next;

    return objp;
}

#define kmem_cache_alloc_one(cachep)                    \
    ({                                                  \
        slab_t *slabp;                                  \
                                                        \
        /* Get slab alloc is to come from. */           \
        {                                               \
            struct list_head *p = cachep->firstnotfull; \
            if (p == &cachep->slabs)                    \
                goto alloc_new_slab;                    \
            slabp = list_entry(p, slab_t, list);        \
        }                                               \
        kmem_cache_alloc_one_tail(cachep, slabp);       \
    })

void kmem_cache_alloc_head(kmem_cache_t *cachep, int flags)
{
}

void *__kmem_cache_alloc(kmem_cache_t *cachep, int flags)
{
    unsigned long save_flags;
    void *objp;

    kmem_cache_alloc_head(cachep, flags);
try_again:
    local_irq_save(save_flags);
#ifdef CONFIG_SMP
    {
        cpucache_t *cc = cc_data(cachep);

        if (cc)
        {
            if (cc->avail)
            {
                STATS_INC_ALLOCHIT(cachep);
                objp = cc_entry(cc)[--cc->avail];
            }
            else
            {
                STATS_INC_ALLOCMISS(cachep);
                objp = kmem_cache_alloc_batch(cachep, flags);
                if (!objp)
                    goto alloc_new_slab_nolock;
            }
        }
        else
        {
            spin_lock(&cachep->spinlock);
            objp = kmem_cache_alloc_one(cachep);
            spin_unlock(&cachep->spinlock);
        }
    }
#else
    objp = kmem_cache_alloc_one(cachep);
#endif
    local_irq_restore(save_flags);
    return objp;
alloc_new_slab:
#ifdef CONFIG_SMP
    spin_unlock(&cachep->spinlock);
alloc_new_slab_nolock:
#endif
    local_irq_restore(save_flags);
    if (kmem_cache_grow(cachep, flags))
        /* Someone may have stolen our objs.  Doesn't matter, we'll
		 * just come back here again.
		 */
        goto try_again;
    return NULL;
}

void *kmem_cache_alloc(kmem_cache_t *cachep, int flags)
{
    return __kmem_cache_alloc(cachep, flags);
}

void kmem_cache_free_one(kmem_cache_t *cachep, void *objp)
{

    slab_t *slabp;

    //CHECK_PAGE(virt_to_page(objp));
    /* reduces memory footprint
	 *
	if (OPTIMIZE(cachep))
		slabp = (void*)((unsigned long)objp&(~(PAGE_SIZE-1)));
	 else
	 */
    slabp = GET_PAGE_SLAB(virt_to_page(objp));

#if DEBUG
    if (cachep->flags & SLAB_DEBUG_INITIAL)
        /* Need to call the slab's constructor so the
		 * caller can perform a verify of its state (debugging).
		 * Called without the cache-lock held.
		 */
        cachep->ctor(objp, cachep, SLAB_CTOR_CONSTRUCTOR | SLAB_CTOR_VERIFY);

    if (cachep->flags & SLAB_RED_ZONE)
    {
        objp -= BYTES_PER_WORD;
        if (xchg((unsigned long *)objp, RED_MAGIC1) != RED_MAGIC2)
            /* Either write before start, or a double free. */
            BUG();
        if (xchg((unsigned long *)(objp + cachep->objsize -
                                   BYTES_PER_WORD),
                 RED_MAGIC1) != RED_MAGIC2)
            /* Either write past end, or a double free. */
            BUG();
    }
    if (cachep->flags & SLAB_POISON)
        kmem_poison_obj(cachep, objp);
    if (kmem_extra_free_checks(cachep, slabp, objp))
        return;
#endif
    {
        unsigned int objnr = (objp - slabp->s_mem) / cachep->objsize;

        slab_bufctl(slabp)[objnr] = slabp->free;
        slabp->free = objnr;
    }
    STATS_DEC_ACTIVE(cachep);

    /* fixup slab chain */
    if (slabp->inuse-- == cachep->num)
        goto moveslab_partial;
    if (!slabp->inuse)
        goto moveslab_free;
    return;

moveslab_partial:
    /* was full.
	 * Even if the page is now empty, we can set c_firstnotfull to
	 * slabp: there are no partial slabs in this case
	 */
    {
        struct list_head *t = cachep->firstnotfull;

        cachep->firstnotfull = &slabp->list;
        if (slabp->list.next == t)
            return;
        list_del(&slabp->list);
        list_add_tail(&slabp->list, t);
        return;
    }
moveslab_free:
    /*
	 * was partial, now empty.
	 * c_firstnotfull might point to slabp
	 * FIXME: optimize
	 */
    {
        struct list_head *t = cachep->firstnotfull->prev;

        list_del(&slabp->list);
        list_add_tail(&slabp->list, &cachep->slabs);
        if (cachep->firstnotfull == &slabp->list)
            cachep->firstnotfull = t->next;
        return;
    }
}

void kmem_freepages(kmem_cache_t *cachep, void *addr)
{
    unsigned long i = (1 << cachep->gfporder);
    struct page *page = virt_to_page(addr);

    /* free_pages() does not clear the type bit - we do that.
	 * The pages have been unlinked from their cache-slab,
	 * but their 'struct page's might be accessed in
	 * vm_scan(). Shouldn't be a worry.
	 */
    while (i--)
    {
        PageClearSlab(page);
        page++;
    }

    free_pages((unsigned long)addr, cachep->gfporder);
}

void __kmem_cache_free(kmem_cache_t *cachep, void *objp)
{
#ifdef CONFIG_SMP
    cpucache_t *cc = cc_data(cachep);

    CHECK_PAGE(virt_to_page(objp));
    if (cc)
    {
        int batchcount;
        if (cc->avail < cc->limit)
        {
            STATS_INC_FREEHIT(cachep);
            cc_entry(cc)[cc->avail++] = objp;
            return;
        }
        STATS_INC_FREEMISS(cachep);
        batchcount = cachep->batchcount;
        cc->avail -= batchcount;
        free_block(cachep,
                   &cc_entry(cc)[cc->avail], batchcount);
        cc_entry(cc)[cc->avail++] = objp;
        return;
    }
    else
    {
        free_block(cachep, &objp, 1);
    }
#else
    kmem_cache_free_one(cachep, objp);
#endif
}


void kmem_cache_free(kmem_cache_t *cachep, void *objp)
{
    unsigned long flags;

    local_irq_save(flags);
    __kmem_cache_free(cachep, objp);
    local_irq_restore(flags);
}

#define BREAK_GFP_ORDER_HI 2
#define BREAK_GFP_ORDER_LO 1
static int slab_break_gfp_order = BREAK_GFP_ORDER_LO;

static unsigned long offslab_limit;

/* internal cache of cache description objs */
static kmem_cache_t cache_cache;

/* Guard access to the cache-chain. */
//static struct semaphore	cache_chain_sem;

/* Place maintainer for reaping. */
//static kmem_cache_t *clock_searchp = &cache_cache;

#define cache_chain (cache_cache.next)

void kmem_cache_estimate(unsigned long gfporder, unsigned int size, int flags, unsigned int *left_over, unsigned int *num)
{
    int i;
    unsigned int wastage = PAGE_SIZE << gfporder;
    unsigned int extra = 0;
    unsigned int base = 0;

    if (!(flags & CFLGS_OFF_SLAB))
    {
        base = sizeof(slab_t);
        extra = sizeof(kmem_bufctl_t);
    }

    i = 0;
    while (i * size + L1_CACHE_ALIGN(base + i * extra) <= wastage)
        i++;
    if (i > 0)
        i--;

    if (i > SLAB_LIMIT)
        i = SLAB_LIMIT;

    *num = i;
    wastage -= i * size;
    wastage -= L1_CACHE_ALIGN(base + i * extra);
    *left_over = wastage;
}

typedef struct cache_sizes
{
    unsigned int cs_size;
    kmem_cache_t *cs_cachep;
    kmem_cache_t *cs_dmacachep;
} cache_sizes_t;

static cache_sizes_t cache_sizes[] =
    {
        {32, NULL, NULL},
        {64, NULL, NULL},
        {128, NULL, NULL},
        {256, NULL, NULL},
        {512, NULL, NULL},
        {1024, NULL, NULL},
        {2048, NULL, NULL},
        {4096, NULL, NULL},
        {8192, NULL, NULL},
        {16384, NULL, NULL},
        {32768, NULL, NULL},
        {65536, NULL, NULL},
        {131072, NULL, NULL},
        {0, NULL, NULL}};

kmem_cache_t *kmem_find_general_cachep(unsigned size, int gfpflags)
{
    cache_sizes_t *csizep = cache_sizes;

    /* This function could be moved to the header file, and
	 * made inline so consumers can quickly determine what
	 * cache pointer they require.
	 */
    for (; csizep->cs_size; csizep++)
    {
        if (size > csizep->cs_size)
            continue;
        break;
    }
    return (gfpflags & GFP_DMA) ? csizep->cs_dmacachep : csizep->cs_cachep;
}

kmem_cache_t *kmem_cache_create(char *name, unsigned int size, unsigned int offset,
                                unsigned long flags, void (*ctor)(void *, kmem_cache_t *, unsigned long),
                                void (*dtor)(void *, kmem_cache_t *, unsigned long))
{
    char *func_nm = "KERNEL ERR kmem_create: ";
    unsigned int left_over, align, slab_size;
    kmem_cache_t *cachep = NULL;

    /*
	 * Sanity checks... these are all serious usage bugs.
	 */
    if ((!name) ||
        ((strlen(name) >= CACHE_NAMELEN - 1)) ||
        //in_interrupt() ||
        (size < BYTES_PER_WORD) ||
        (size > (1 << MAX_OBJ_ORDER) * PAGE_SIZE) ||
        (dtor && !ctor) ||
        (offset < 0 || offset > size))
        BUG();

    /*
	 * Always checks flags, a caller might be expecting debug
	 * support which isn't available.
	 */
    if (flags & ~CREATE_MASK)
        BUG();

    /* Get cache's description obj. */
    cachep = (kmem_cache_t *)kmem_cache_alloc(&cache_cache, SLAB_KERNEL);
    if (!cachep)
        goto opps;
    memset(cachep, 0, sizeof(kmem_cache_t));

    /* Check that size is in terms of words.  This is needed to avoid
	 * unaligned accesses for some archs when redzoning is used, and makes
	 * sure any on-slab bufctl's are also correctly aligned.
	 */
    if (size & (BYTES_PER_WORD - 1))
    {
        size += (BYTES_PER_WORD - 1);
        size &= ~(BYTES_PER_WORD - 1);
        printk("%sForcing size word alignment - %s\n", func_nm, name);
    }

    align = BYTES_PER_WORD;
    if (flags & SLAB_HWCACHE_ALIGN)
        align = L1_CACHE_BYTES;

    /* Determine if the slab management is 'on' or 'off' slab. */
    if (size >= (PAGE_SIZE >> 3))
        /*
		 * Size is large, assume best to place the slab management obj
		 * off-slab (should allow better packing of objs).
		 */
        flags |= CFLGS_OFF_SLAB;

    if (flags & SLAB_HWCACHE_ALIGN)
    {
        /* Need to adjust size so that objs are cache aligned. */
        /* Small obj size, can get at least two per cache line. */
        /* FIXME: only power of 2 supported, was better */
        while (size < align / 2)
            align /= 2;
        size = (size + align - 1) & (~(align - 1));
    }

    /* Cal size (in pages) of slabs, and the num of objs per slab.
	 * This could be made much more intelligent.  For now, try to avoid
	 * using high page-orders for slabs.  When the gfp() funcs are more
	 * friendly towards high-order requests, this should be changed.
	 */
    do
    {
        unsigned int break_flag = 0;
    cal_wastage:
        kmem_cache_estimate(cachep->gfporder, size, flags,
                            &left_over, &cachep->num);
        if (break_flag)
            break;
        if (cachep->gfporder >= MAX_GFP_ORDER)
            break;
        if (!cachep->num)
            goto next;
        if (flags & CFLGS_OFF_SLAB && cachep->num > offslab_limit)
        {
            /* Oops, this num of objs will cause problems. */
            cachep->gfporder--;
            break_flag++;
            goto cal_wastage;
        }

        /*
		 * Large num of objs is good, but v. large slabs are currently
		 * bad for the gfp()s.
		 */
        if (cachep->gfporder >= slab_break_gfp_order)
            break;

        if ((left_over * 8) <= (PAGE_SIZE << cachep->gfporder))
            break; /* Acceptable internal fragmentation. */
    next:
        cachep->gfporder++;
    } while (1);

    if (!cachep->num)
    {
        printk("kmem_cache_create: couldn't create cache %s.\n", name);
        kmem_cache_free(&cache_cache, cachep);
        cachep = NULL;
        goto opps;
    }
    slab_size = L1_CACHE_ALIGN(cachep->num * sizeof(kmem_bufctl_t) + sizeof(slab_t));

    /*
	 * If the slab has been placed off-slab, and we have enough space then
	 * move it on-slab. This is at the expense of any extra colouring.
	 */
    if (flags & CFLGS_OFF_SLAB && left_over >= slab_size)
    {
        flags &= ~CFLGS_OFF_SLAB;
        left_over -= slab_size;
    }

    /* Offset must be a multiple of the alignment. */
    offset += (align - 1);
    offset &= ~(align - 1);
    if (!offset)
        offset = L1_CACHE_BYTES;
    cachep->colour_off = offset;
    cachep->colour = left_over / offset;

    /* init remaining fields */
    if (!cachep->gfporder && !(flags & CFLGS_OFF_SLAB))
        flags |= CFLGS_OPTIMIZE;

    cachep->flags = flags;
    cachep->gfpflags = 0;
    if (flags & SLAB_CACHE_DMA)
        cachep->gfpflags |= GFP_DMA;
    spin_lock_init(&cachep->spinlock);
    cachep->objsize = size;
    INIT_LIST_HEAD(&cachep->slabs);
    cachep->firstnotfull = &cachep->slabs;

    if (flags & CFLGS_OFF_SLAB)
        cachep->slabp_cache = kmem_find_general_cachep(slab_size, 0);
    cachep->ctor = ctor;
    cachep->dtor = dtor;
    /* Copy name over so we don't have problems with unloaded modules */
    strcpy(cachep->name, name);

#ifdef CONFIG_SMP
    if (g_cpucache_up)
        enable_cpucache(cachep);
#endif
    /* Need the semaphore to access the chain. */
    //  down(&cache_chain_sem);
    {
        struct list_head *p;

        list_for_each(p, &cache_chain)
        {
            kmem_cache_t *pc = list_entry(p, kmem_cache_t, next);

            /* The name field is constant - no lock needed. */
            if (!strcmp(pc->name, name))
                BUG();
        }
    }

    /* There is no reason to lock our new cache before we
	 * link it in - no one knows about it yet...
	 */
    list_add(&cachep->next, &cache_chain);
    //up(&cache_chain_sem);
opps:
    return cachep;
}

void kmem_cache_init(void)
{
    unsigned int left_over;

    cache_cache.slabs.next = &cache_cache.slabs;
    cache_cache.slabs.prev = &cache_cache.slabs;
    cache_cache.firstnotfull = &cache_cache.slabs;
    cache_cache.objsize = sizeof(kmem_cache_t);
    cache_cache.flags = SLAB_NO_REAP;
    cache_cache.spinlock = SPIN_LOCK_UNLOCKED;
    cache_cache.colour_off = L1_CACHE_BYTES;
    strcpy(cache_cache.name, "kmem_cache");

    //init_MUTEX(&cache_chain_sem);
    INIT_LIST_HEAD(&cache_chain);

    kmem_cache_estimate(0, cache_cache.objsize, 0, &left_over, &cache_cache.num);

    if (!cache_cache.num)
        BUG();

    cache_cache.colour = left_over / cache_cache.colour_off;
    cache_cache.colour_next = 0;
}

void kmem_cache_sizes_init(void)
{
    cache_sizes_t *sizes = cache_sizes;
    char name[20];
    /*
	 * Fragmentation resistance on low memory - only use bigger
	 * page orders on machines with more than 32MB of memory.
	 */
    if (num_physpages > (32 << 20) >> PAGE_SHIFT)
        slab_break_gfp_order = BREAK_GFP_ORDER_HI;
    do 
    {
        /* For performance, all the general caches are L1 aligned.
		 * This should be particularly beneficial on SMP boxes, as it
		 * eliminates "false sharing".
		 * Note for systems short on memory removing the alignment will
		 * allow tighter packing of the smaller caches. */
        sprintk(name, "size-%d", sizes->cs_size);
        if (!(sizes->cs_cachep = kmem_cache_create(name, sizes->cs_size, 0, SLAB_HWCACHE_ALIGN, NULL, NULL)))
        {
            BUG();
        }

        /* Inc off-slab bufctl limit until the ceiling is hit. */
        if (!(OFF_SLAB(sizes->cs_cachep)))
        {
            offslab_limit = sizes->cs_size - sizeof(slab_t);
            offslab_limit /= 2;
        }
#if 0
        sprintk(name, "size-%d(DMA)", sizes->cs_size);
        sizes->cs_dmacachep = kmem_cache_create(name, sizes->cs_size, 0, SLAB_CACHE_DMA | SLAB_HWCACHE_ALIGN, NULL, NULL);
        if (!sizes->cs_dmacachep)
            BUG();
#endif
        sizes++;
    } while (sizes->cs_size);
}

#if DEBUG
#define CHECK_NR(pg)                                           \
    do                                                         \
    {                                                          \
        if (!VALID_PAGE(pg))                                   \
        {                                                      \
            printk(KERN_ERR "kfree: out of range ptr %lxh.\n", \
                   (unsigned long)objp);                       \
            BUG();                                             \
        }                                                      \
    } while (0)
#define CHECK_PAGE(page)                              \
    do                                                \
    {                                                 \
        CHECK_NR(page);                               \
        if (!PageSlab(page))                          \
        {                                             \
            printk(KERN_ERR "kfree: bad ptr %lxh.\n", \
                   (unsigned long)objp);              \
            BUG();                                    \
        }                                             \
    } while (0)

#else
#define CHECK_PAGE(pg) \
    do                 \
    {                  \
    } while (0)
#endif

void *my_kmalloc(unsigned int size, int flags)
{
    cache_sizes_t *csizep = cache_sizes;

    for (; csizep->cs_size; csizep++)
    {
        if (size > csizep->cs_size)
            continue;
        return __kmem_cache_alloc(flags & GFP_DMA ? csizep->cs_dmacachep : csizep->cs_cachep, flags);
    }
    BUG(); // too big size
    return NULL;
}

void my_kfree(const void *objp)
{
    kmem_cache_t *c;
    unsigned long flags;

    if (!objp)
        return;
    local_irq_save(flags);
    CHECK_PAGE(virt_to_page(objp));
    c = GET_PAGE_CACHE(virt_to_page(objp));
    __kmem_cache_free(c, (void *)objp);
    local_irq_restore(flags);
}
