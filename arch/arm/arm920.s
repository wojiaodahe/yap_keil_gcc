    /*
 * This is the maximum size of an area which will be invalidated
 * using the single invalidate entry instructions.  Anything larger
 * than this, and we go for the whole cache.
 *
 * This value should be chosen such that we choose the cheapest
 * alternative.
 */
.equ MAX_AREA_SIZE,	16384

/*
 * the cache line size of the I and D cache
 */
 .equ DCACHELINESIZE,	32
 .equ ICACHELINESIZE,	32

/*
 * and the page size
 */
.equ PAGESIZE,	4096

.equ LPTE_PRESENT,       (1 << 0)
.equ LPTE_YOUNG,         (1 << 1) 
.equ LPTE_BUFFERABLE,    (1 << 2)
.equ LPTE_CACHEABLE,     (1 << 3)
.equ LPTE_USER,          (1 << 4)
.equ LPTE_WRITE,         (1 << 5)
.equ LPTE_EXEC,          (1 << 6) 
.equ LPTE_DIRTY,         (1 << 7) 

.equ HPTE_TYPE_SMALL,    0x0002
.equ HPTE_AP_READ,       0x0aa0
.equ HPTE_AP_WRITE,      0x0550

.global cpu_arm920_set_pgd
.global cpu_arm920_set_pte
.global cpu_arm920_cache_clean_invalidate_all
.global cpu_arm920_tlb_invalidate_all
.global cpu_arm920_clean_dentry_and_drain_Wb

	.text

/*
 * cpu_arm920_data_abort()
 *
 * obtain information about current aborted instruction
 *
 * r0 = address of aborted instruction
 *
 * Returns:
 *  r0 = address of abort
 *  r1 != 0 if writing
 *  r3 = FSR
 */
	.align	5
cpu_arm920_data_abort:
	ldr	r1, [r0]			@ read aborted instruction
	mrc	p15, 0, r0, c6, c0, 0		@ get FAR
	mov	r1, r1, lsr #19			@ b1 = L
	mrc	p15, 0, r3, c5, c0, 0		@ get FSR
	and	r1, r1, #2
	and	r3, r3, #255
	mov	pc, lr

/* ================================= CACHE ================================ */


/*
 * cpu_arm920_cache_clean_invalidate_all()
 *
 * clean and invalidate all cache lines
 *
 * Note:
 *  1. we should preserve r0 at all times
 */
	.align	5
cpu_arm920_cache_clean_invalidate_all:
	mov	r2, #1
cpu_arm920_cache_clean_invalidate_all_r2:
	mov	ip, #0
#ifdef CONFIG_CPU_ARM920_FORCE_WRITE_THROUGH
	mcr	p15, 0, ip, c7, c6, 0		@ invalidate D cache
#else
/*
 * 'Clean & Invalidate whole DCache'
 * Re-written to use Index Ops.
 * Uses registers r1, r3 and ip
 */
	mov	r1, #7 << 5			@ 8 segments
1:	orr	r3, r1, #63 << 26		@ 64 entries
2:	mcr	p15, 0, r3, c7, c14, 2		@ clean & invalidate D index
	subs	r3, r3, #1 << 26
	bcs	2b				@ entries 63 to 0
	subs	r1, r1, #1 << 5
	bcs	1b				@ segments 7 to 0
#endif
	teq	r2, #0
	mcrne	p15, 0, ip, c7, c5, 0		@ invalidate I cache
	mcr	p15, 0, ip, c7, c10, 4		@ drain WB
	mov	pc, lr

/*
 * cpu_arm920_cache_clean_invalidate_range(start, end, flags)
 *
 * clean and invalidate all cache lines associated with this area of memory
 *
 * start: Area start address
 * end:   Area end address
 * flags: nonzero for I cache as well
 */
	.align	5
cpu_arm920_cache_clean_invalidate_range:
	bic	r0, r0, #DCACHELINESIZE - 1	@ && added by PGM
	sub	r3, r1, r0
	cmp	r3, #MAX_AREA_SIZE
	bgt	cpu_arm920_cache_clean_invalidate_all_r2
1:	mcr	p15, 0, r0, c7, c14, 1		@ clean and invalidate D entry
	add	r0, r0, #DCACHELINESIZE
	mcr	p15, 0, r0, c7, c14, 1		@ clean and invalidate D entry
	add	r0, r0, #DCACHELINESIZE
	cmp	r0, r1
	blt	1b
	teq	r2, #0
	movne	r0, #0
	mcrne	p15, 0, r0, c7, c5, 0		@ invalidate I cache
	mov	pc, lr

/*
 * cpu_arm920_flush_ram_page(page)
 *
 * clean and invalidate all cache lines associated with this area of memory
 *
 * page: page to clean and invalidate
 */
	.align	5
cpu_arm920_flush_ram_page:
	mov	r1, #PAGESIZE
1:	mcr	p15, 0, r0, c7, c14, 1		@ clean and invalidate D entry
	add	r0, r0, #DCACHELINESIZE
	mcr	p15, 0, r0, c7, c14, 1		@ clean and invalidate D entry
	add	r0, r0, #DCACHELINESIZE
	subs	r1, r1, #2 * DCACHELINESIZE
	bne	1b
	mcr	p15, 0, r1, c7, c10, 4		@ drain WB
	mov	pc, lr

/* ================================ D-CACHE =============================== */

/*
 * cpu_arm920_dcache_invalidate_range(start, end)
 *
 * throw away all D-cached data in specified region without an obligation
 * to write them back.  Note however that we must clean the D-cached entries
 * around the boundaries if the start and/or end address are not cache
 * aligned.
 *
 * start: virtual start address
 * end:   virtual end address
 */
	.align	5
cpu_arm920_dcache_invalidate_range:
	tst	r0, #DCACHELINESIZE - 1
	bic	r0, r0, #DCACHELINESIZE - 1
	mcrne	p15, 0, r0, c7, c10, 1		@ clean D entry
	tst	r1, #DCACHELINESIZE - 1
	mcrne	p15, 0, r1, c7, c10, 1		@ clean D entry
1:	mcr	p15, 0, r0, c7, c6, 1		@ invalidate D entry
	add	r0, r0, #DCACHELINESIZE
	cmp	r0, r1
	blt	1b
	mov	pc, lr

/*
 * cpu_arm920_dcache_clean_range(start, end)
 *
 * For the specified virtual address range, ensure that all caches contain
 * clean data, such that peripheral accesses to the physical RAM fetch
 * correct data.
 *
 * start: virtual start address
 * end:   virtual end address
 */
	.align	5
cpu_arm920_dcache_clean_range:
	bic	r0, r0, #DCACHELINESIZE - 1
	sub	r1, r1, r0
	cmp	r1, #MAX_AREA_SIZE
	mov	r2, #0
	bgt	cpu_arm920_cache_clean_invalidate_all_r2

1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	add	r0, r0, #DCACHELINESIZE
	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	add	r0, r0, #DCACHELINESIZE
	subs	r1, r1, #2 * DCACHELINESIZE
	bpl	1b
	mcr	p15, 0, r2, c7, c10, 4		@ drain WB
	mov	pc, lr

/*
 * cpu_arm920_dcache_clean_page(page)
 *
 * Cleans a single page of dcache so that if we have any future aliased
 * mappings, they will be consistent at the time that they are created.
 *
 * page: virtual address of page to clean from dcache
 *
 * Note:
 *  1. we don't need to flush the write buffer in this case.
 *  2. we don't invalidate the entries since when we write the page
 *     out to disk, the entries may get reloaded into the cache.
 */
	.align	5
cpu_arm920_dcache_clean_page:
#ifndef CONFIG_CPU_ARM920_FORCE_WRITE_THROUGH
	mov	r1, #PAGESIZE
1:	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	add	r0, r0, #DCACHELINESIZE
	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	add	r0, r0, #DCACHELINESIZE
	subs	r1, r1, #2 * DCACHELINESIZE
	bne	1b
#endif
	mov	pc, lr

/*
 * cpu_arm920_dcache_clean_entry(addr)
 *
 * Clean the specified entry of any caches such that the MMU
 * translation fetches will obtain correct data.
 *
 * addr: cache-unaligned virtual address
 */
	.align	5
cpu_arm920_dcache_clean_entry:
#ifndef CONFIG_CPU_ARM920_FORCE_WRITE_THROUGH
	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
#endif
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

/* ================================ I-CACHE =============================== */

/*
 * cpu_arm920_icache_invalidate_range(start, end)
 *
 * invalidate a range of virtual addresses from the Icache
 *
 * start: virtual start address
 * end:   virtual end address
 */
	.align	5
cpu_arm920_icache_invalidate_range:
1:	mcr	p15, 0, r0, c7, c10, 1		@ Clean D entry
	add	r0, r0, #DCACHELINESIZE
	cmp	r0, r1
	blo	1b
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
cpu_arm920_icache_invalidate_page:
	/* why no invalidate I cache --rmk */
	mov	pc, lr


/* ================================== TLB ================================= */

/*
 * cpu_arm920_tlb_invalidate_all()
 *
 * Invalidate all TLB entries
 */
	.align	5
cpu_arm920_tlb_invalidate_all:
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mcr	p15, 0, r0, c8, c7, 0		@ invalidate I & D TLBs
	mov	pc, lr

/*
 * cpu_arm920_tlb_invalidate_range(start, end)
 *
 * invalidate TLB entries covering the specified range
 *
 * start: range start address
 * end:   range end address
 */
	.align	5
cpu_arm920_tlb_invalidate_range:
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 4		@ drain WB
1:	mcr	p15, 0, r0, c8, c6, 1		@ invalidate D TLB entry
	mcr	p15, 0, r0, c8, c5, 1		@ invalidate I TLB entry
	add	r0, r0, #PAGESIZE
	cmp	r0, r1
	blt	1b
	mov	pc, lr

/*
 * cpu_arm920_tlb_invalidate_page(page, flags)
 *
 * invalidate the TLB entries for the specified page.
 *
 * page:  page to invalidate
 * flags: non-zero if we include the I TLB
 */
	.align	5
cpu_arm920_tlb_invalidate_page:
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 4		@ drain WB
	teq	r1, #0
	mcr	p15, 0, r0, c8, c6, 1		@ invalidate D TLB entry
	mcrne	p15, 0, r0, c8, c5, 1		@ invalidate I TLB entry
	mov	pc, lr

/* =============================== PageTable ============================== */

/*
 * cpu_arm920_set_pgd(pgd)
 *
 * Set the translation base pointer to be as described by pgd.
 *
 * pgd: new page tables
 */
	.align	5
cpu_arm920_set_pgd:
	mov	ip, #0
#ifdef CONFIG_CPU_ARM920_FORCE_WRITE_THROUGH
	/* Any reason why we don't use mcr p15, 0, r0, c7, c7, 0 here? --rmk */
@	mcr	p15, 0, ip, c7, c6, 0		@ invalidate D cache
#else
@ && 'Clean & Invalidate whole DCache'
@ && Re-written to use Index Ops.
@ && Uses registers r1, r3 and ip

	mov	r1, #7 << 5			@ 8 segments
1:	orr	r3, r1, #63 << 26		@ 64 entries
2:	mcr	p15, 0, r3, c7, c14, 2		@ clean & invalidate D index
	subs	r3, r3, #1 << 26
	bcs	2b				@ entries 63 to 0
	subs	r1, r1, #1 << 5
	bcs	1b				@ segments 7 to 0
#endif
	mcr	p15, 0, ip, c7, c5, 0		@ invalidate I cache
	mcr	p15, 0, ip, c7, c10, 4		@ drain WB
	mcr	p15, 0, r0, c2, c0, 0		@ load page table pointer
	mcr	p15, 0, ip, c8, c7, 0		@ invalidate I & D TLBs
	mov	pc, lr

/*
 * cpu_arm920_set_pmd(pmdp, pmd)
 *
 * Set a level 1 translation table entry, and clean it out of
 * any caches such that the MMUs can load it correctly.
 *
 * pmdp: pointer to PMD entry
 * pmd:  PMD value to store
 */
	.align	5
cpu_arm920_set_pmd:
#ifdef CONFIG_CPU_ARM920_FORCE_WRITE_THROUGH
	eor	r2, r1, #0x0a			@ C & Section
	tst	r2, #0x0b
	biceq	r1, r1, #4			@ clear bufferable bit
#endif
	str	r1, [r0]
	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

/*
 * cpu_arm920_set_pte(ptep, pte)
 *
 * Set a PTE and flush it out
 */
	.align	5
cpu_arm920_set_pte:
	str	r1, [r0], #-1024		@ linux version

	eor	r1, r1, #(LPTE_PRESENT | LPTE_YOUNG | LPTE_WRITE | LPTE_DIRTY)

	bic	r2, r1, #0xff0
	bic	r2, r2, #3
	orr	r2, r2, #HPTE_TYPE_SMALL

	tst	r1, #(LPTE_USER | LPTE_EXEC)	@ User or Exec?
	orrne	r2, r2, #HPTE_AP_READ

	tst	r1, #(LPTE_WRITE | LPTE_DIRTY)	@ Write and Dirty?
	orreq	r2, r2, #HPTE_AP_WRITE

	tst	r1, #(LPTE_PRESENT | LPTE_YOUNG)	@ Present and Young?
	movne	r2, #0

#ifdef CONFIG_CPU_ARM920_FORCE_WRITE_THROUGH
	eor	r3, r1, #0x0a			@ C & small page?
	tst	r3, #0x0b
	biceq	r2, r2, #4
#endif
	str	r2, [r0]			@ hardware version
	mov	r0, r0
	mcr	p15, 0, r0, c7, c10, 1		@ clean D entry
	mcr	p15, 0, r0, c7, c10, 4		@ drain WB
	mov	pc, lr

cpu_arm920_clean_dentry_and_drain_Wb:
	mov r0, #0
	
	mov	r1, #7 << 5			@ 8 segments
1:	orr	r3, r1, #63 << 26		@ 64 entries
2:	mcr	p15, 0, r3, c7, c14, 2		@ clean & invalidate D index
	subs	r3, r3, #1 << 26
	bcs	2b				@ entries 63 to 0
	subs	r1, r1, #1 << 5
	bcs	1b				@ segments 7 to 0

	mcr	p15, 0, r0, c7, c10, 4		@ drain WB

	mov	pc, lr

