#ifndef __ARM_MEMORY_H__
#define __ARM_MEMORY_H__

#define PHYS_OFFSET	(0x30000000)

#if 0
#define __virt_to_phys(x)	((x) - PAGE_OFFSET + PHYS_OFFSET)
#define __phys_to_virt(x)	((x) - PHYS_OFFSET + PAGE_OFFSET)
#else
#define __virt_to_phys(x)	(x)
#define __phys_to_virt(x)	(x)
#endif

#define PHYS_PFN_OFFSET	    (PHYS_OFFSET >> PAGE_SHIFT)
#define ARCH_PFN_OFFSET		PHYS_PFN_OFFSET

#define __pfn_to_page(pfn)	(mem_map + ((pfn) - ARCH_PFN_OFFSET))
#define __page_to_pfn(page)	((unsigned long)((page) - mem_map) +  ARCH_PFN_OFFSET)


#define page_to_pfn         __page_to_pfn
#define pfn_to_page         __pfn_to_page

#define __pa(x)			    __virt_to_phys((unsigned long)(x))
#define __va(x)			    ((void *)__phys_to_virt((unsigned long)(x)))
#define pfn_to_kaddr(pfn)	__va((pfn) << PAGE_SHIFT)

#define virt_to_page(kaddr)	pfn_to_page(__pa(kaddr) >> PAGE_SHIFT)

#define TASK_SIZE   (PAGE_OFFSET)

#endif

