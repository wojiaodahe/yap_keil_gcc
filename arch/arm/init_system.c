#include "lib.h"
#include "s3c24xx.h"
#include "system.h"
#include "config.h"
#include "proc.h"
#include "common.h"
#include "interrupt.h"
#include "s3c24xx_irqs.h"

extern void enable_irq(void);
extern void disable_irq(void);

/*
 * ??WATCHDOG,??CPU?????
 */
void disable_watch_dog(void)
{
    WTCON = 0;
}


void init_memory(void)
{
    int i = 0;
    volatile unsigned long *p = (volatile unsigned long *)MEM_CTL_BASE;

    /* SDRAM 13?????? */
    unsigned long  const    mem_cfg_val[] = {
					      0x22000000,     //BWSCON
					      0x00000700,     //BANKCON0
					      0x00000700,     //BANKCON1
					      0x00000700,     //BANKCON2
					      0x00000700,     //BANKCON3
					      0x00000700,     //BANKCON4
					      0x00000700,     //BANKCON5
					      0x00018001,     //BANKCON6
					      0x00018001,     //BANKCON7
					      0x008404f5,     //REFRESH
					      0x000000B1,     //BANKSIZE
					      0x00000020,     //MRSRB6
					      0x00000020,     //MRSRB7
					    };


    for (; i < 13; i++)
        p[i] = mem_cfg_val[i];
}


/*
 * ????
 */

extern unsigned int MMU_TLB_BASE;
extern unsigned int SYSTEM_TOTAL_MEMORY_START;

 #define VECTOR_BASE SYSTEM_TOTAL_MEMORY_START
//??1M????
void __set_l1_section_descriptor(unsigned long virtuladdr, unsigned long physicaladdr, unsigned int attributes)
{
    volatile unsigned int *mmu_tlb_base = (unsigned int*)MMU_TLB_BASE;

	*(mmu_tlb_base + (virtuladdr >> 20)) = (physicaladdr & 0xFFF00000) | attributes;
}

//????n M????
void set_l1_parallel_descriptor(unsigned long viraddr_start, unsigned long viraddr_end, unsigned long phyaddr_start, unsigned int attributes)
{
	int nSec, i;
	volatile unsigned long *mmu_tlb_base;

	mmu_tlb_base = (unsigned long *)MMU_TLB_BASE + (viraddr_start >> 20);
	nSec = (viraddr_end >> 20) - (viraddr_start >> 20);

	for (i = 0; i < nSec; i++)
		*mmu_tlb_base++ = attributes | (((phyaddr_start >> 20) + i) << 20);
}

void create_page_table(void)
{

    unsigned long virtuladdr, physicaladdr;
    volatile unsigned long *mmu_tlb_base = (volatile unsigned long *)MMU_TLB_BASE;

    /*
     * ?0~1M????????0x30000000
     */
    virtuladdr = 0;
    physicaladdr = VECTOR_BASE;
    *(mmu_tlb_base + (virtuladdr >> 20)) = (physicaladdr & 0xFFF00000) | \
                                            MMU_SECDESC_WB;
	
    virtuladdr = 0x20000000;
    physicaladdr = 0x20000000;
    *(mmu_tlb_base + (virtuladdr >> 20)) = (physicaladdr & 0xFFF00000) | \
                                            MMU_SECDESC_WB_NCNB;
   

	set_l1_parallel_descriptor(0x30000000, 0x34000000, 0x30000000, MMU_SECDESC_WB);

    virtuladdr = 0x50000000;
    physicaladdr = 0x50000000;
    *(mmu_tlb_base + (virtuladdr >> 20)) = (physicaladdr & 0xFFF00000) | \
                                            MMU_SECDESC;
    virtuladdr = 0x51000000;
    physicaladdr = 0x51000000;
    *(mmu_tlb_base + (virtuladdr >> 20)) = (physicaladdr & 0xFFF00000) | \
                                            MMU_SECDESC;
    /*
     * 0x56000000?GPIO??????????,
     */
    virtuladdr = 0x56000000;
    physicaladdr = 0x56000000;
    *(mmu_tlb_base + (virtuladdr >> 20)) = (physicaladdr & 0xFFF00000) | \
                                            MMU_SECDESC;
    
    virtuladdr = 0x59000000;
    physicaladdr = 0x59000000;
    *(mmu_tlb_base + (virtuladdr >> 20)) = (physicaladdr & 0xFFF00000) | \
                                            MMU_SECDESC;
    
	virtuladdr = 0x48000000;
    physicaladdr = 0x48000000;
    *(mmu_tlb_base + (virtuladdr >> 20)) = (physicaladdr & 0xFFF00000) | \
                                            MMU_SECDESC;


    virtuladdr = 0x4a000000;
    physicaladdr = 0x4a000000;
    *(mmu_tlb_base + (virtuladdr >> 20)) = (physicaladdr & 0xFFF00000) | \
                                            MMU_SECDESC;

    virtuladdr = 0x4e000000;
    physicaladdr = 0x4e000000;
    *(mmu_tlb_base + (virtuladdr >> 20)) = (physicaladdr & 0xFFF00000) | \
                                            MMU_SECDESC;

}

/*
 * ??MMU
 */

void start_mmu(void)
{
    unsigned long ttb = MMU_TLB_BASE;

__asm__(

        "mov    r0, #0\n"
        "mcr    p15, 0, r0, c7, c7, 0\n" 
                                         		
        "mcr    p15, 0, r0, c7, c10, 4\n"	   /* ???ICaches?DCaches */
        "mcr    p15, 0, r0, c8, c7, 0\n" 
                                         	   /* drain write buffer on v4 */
        "mov    r4, %0\n"                	    /* ????????TLB */
        "mcr    p15, 0, r4, c2, c0, 0\n" 
                                         	   /* r4 = ???? */
        "mvn    r0, #0\n"                	   /* ????????? */
        "mcr    p15, 0, r0, c3, c0, 0\n"       /* ??????????0xFFFFFFFF,
					                           * ???????*/
		/*
		* ???????,?????,????????????,
		* ?????
		*/
		"mrc    p15, 0, r0, c1, c0, 0\n"       /* ????????? */

		/* ???????16????:.RVI ..RS B... .CAM
		* R : ????Cache??????????,
		*     0 = Random replacement;1 = Round robin replacement
		* V : ????????????,
		*     0 = Low addresses = 0x00000000;1 = High addresses = 0xFFFF0000
		* I : 0 = ??ICaches;1 = ??ICaches
		* R?S : ?????????????????????
		* B : 0 = CPU?????;1 = CPU?????
		* C : 0 = ??DCaches;1 = ??DCaches
		* A : 0 = ??????????????;1 = ?????????????
		* M : 0 = ??MMU;1 = ??MMU
		*/

		/*
		* ????????,????????????
		*/
		                              /* .RVI ..RS B... .CAM */
		"bic    r0, r0, #0x3000\n"    /* ..11 .... .... .... ??V?I? */
		"bic    r0, r0, #0x0300\n"    /* .... ..11 .... .... ??R?S? */
		"bic    r0, r0, #0x0087\n"    /* .... .... 1... .111 ??B/C/A/M */

		/*
		* ??????
		*/
		"orr    r0, r0, #0x0002\n"   /* .... .... .... ..1. ?????? */
		"orr    r0, r0, #0x0004\n"   /* .... .... .... .1.. ??DCaches */
		"orr    r0, r0, #0x1000\n"   /* ...1 .... .... .... ??ICaches */
		"orr    r0, r0, #0x0001\n"   /* .... .... .... ...1 ??MMU */

        "mcr    p15, 0, r0, c1, c0, 0\n" /* ???????????? */		
        : 
        : "r" (ttb) 
        : "r0"
        );
}

void stop_mmu(void)
{
	__asm__(
		"mrc    p15, 0, r0, c1, c0, 0\n"    /* 闂佽法鍠愰弸濠氬箯閻戣姤鏅搁柡鍌樺€栫€氬綊鏌ㄩ悢鍛婄伄闁归鍏橀弫鎾绘偑閳ュ磭妲戦弶鍫㈠亾鐎氬綊鏌ㄩ悢鍛婄伄闁归鍏橀弫鎾诲棘閵堝棗顏堕柛濠忔嫹 */

		                                /* .RVI ..RS B... .CAM */
		"bic    r0, r0, #0x3000\n"          /* ..11 .... .... .... 闂佽法鍠愰弸濠氬箯閻戣姤鏅哥紒鎻掔Ч閺佹捇寮妶鍡楊伓I濞达綇鎷� */
		"bic    r0, r0, #0x0300\n"          /* .... ..11 .... .... 闂佽法鍠愰弸濠氬箯閻戣姤鏅哥紒鎻掝樀閺佹捇寮妶鍡楊伓S濞达綇鎷� */
		"bic    r0, r0, #0x0087\n"          /* .... .... 1... .111 闂佽法鍠愰弸濠氬箯閻戣姤鏅哥紒鎲嬫嫹/C/A/M */
		"bic    r0, r0, #0x0001\n"          /* .... .... .... ...1 闂佽法鍠愰弸濠氬箯闁垮鍓綧MU */

		"mcr    p15, 0, r0, c1, c0, 0\n"    /* 闂佽法鍠愰弸濠氬箯閻戣姤鏅搁柣顐亝閺佽偐鍠婃径瀣伓闁稿﹦鍘ч崯鎾绘煥閻斿憡鐏柟椋庡厴閺佹捇寮妶鍡楊伓闁解偓瀹ュ棗顎栭梺璺ㄥ枑閺嬪骞忛悜鑺ユ櫢闁跨噦鎷� */		
    );
}

/*
 * ??MPLLCON???,[19:12]?MDIV,[9:4]?PDIV,[1:0]?SDIV
 * ???????:
 *  S3C2410: MPLL(FCLK) = (m * Fin)/(p * 2^s)
 *  S3C2440: MPLL(FCLK) = (2 * m * Fin)/(p * 2^s)
 *  ??: m = MDIV + 8, p = PDIV + 2, s = SDIV
 * ??????,Fin = 12MHz
 * ??CLKDIVN,?????:FCLK:HCLK:PCLK=1:2:4,
 * FCLK=200MHz,HCLK=100MHz,PCLK=50MHz
 */
void init_clock(void)
{
    // LOCKTIME = 0x00ffffff;   // ???????
    CLKDIVN  = 0x05;            // FCLK:HCLK:PCLK=1:2:4, HDIVN=1,PDIVN=1
    
    /* ??HDIVN?0,CPU????????闂備浇娉曢崰鎾剁矉瀹稿兒t bus mode闂備浇娉曢崳锕傚箯閿燂拷??闂備浇娉曢崰鎾剁矉閸х殧nchronous bus mode闂備浇娉曢崳锕傚箯閿燂拷 */
__asm__(
    "mrc    p15, 0, r1, c1, c0, 0\n"        /* ??????? */
    "orr    r1, r1, #0xc0000000\n"          /* ???闂備浇娉曢崰鎾剁矉閸х殧nchronous bus mode闂備浇娉曢崳锕傚箯閿燂拷 */
    "mcr    p15, 0, r1, c1, c0, 0\n"        /* ??????? */
    :
    :
    :"r1"
    );

    /* ???S3C2410??S3C2440 */
    if ((GSTATUS1 == 0x32410000) || (GSTATUS1 == 0x32410002))
    {
        MPLLCON = S3C2410_MPLL_200MHZ;  /* ??,FCLK=200MHz,HCLK=100MHz,PCLK=50MHz */
    }
    else
    {
        MPLLCON = S3C2440_MPLL_200MHZ;  /* ??,FCLK=200MHz,HCLK=100MHz,PCLK=50MHz */
    }
}

void s3c24xx_timer4_irq_handler(void *prv)
{
    OS_Clock_Tick(NULL);
}

int s3c24xx_timer_init(void)
{
	TCFG0 |= (100 << 8);
    TCFG1 |= (2 << 16);
	TCON &= (~(7 << 20));
	TCON |= (1 << 22);
	TCON |= (1 << 21);

	TCONB4 = 625;
	TCON |= (1 << 20);
	TCON &= ~(1 << 21);

    return request_irq(IRQ_TIMER4, s3c24xx_timer4_irq_handler, 0, 0);
}




int init_system(void)
{

    disable_watch_dog();

    init_clock();

    memset((void *)TLB_BASE, 0, 0x10000);

    create_page_table();
    start_mmu();

    //init_memory();

    return 0;
}
