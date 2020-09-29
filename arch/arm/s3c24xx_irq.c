#include "s3c24xx.h"
#include "interrupt.h"
#include "s3c24xx_irqs.h"
#include "printk.h"
#include "kmalloc.h"
#include "s3c24xx_irqs.h"

inline void enable_irq(void)
{
  __asm__(
            "mrs r0, cpsr\n"
            "bic r0, r0, #0x80\n"
            "msr cpsr_cxsf, r0\n"
            :
            :
            :"r0"
           );
}

inline void disable_irq(void)
{
 __asm__(
            "mrs r0, cpsr\n"
            "orr r0, r0, #0x80\n"
            "msr cpsr_cxsf, r0\n"
            :
            :
            :"r0"
           );
}

inline unsigned long arch_local_irq_save(void)
{
      unsigned long flags = 0;

    __asm__ __volatile__(
            "mrs r1, cpsr\n"
            "mov %0, r1\n"
            "orr r1, r1, #0x80\n"
            "msr cpsr_cxsf, r1\n"
            : "=r" (flags)
            :
            : "r1"
           );

    return flags;
}

inline unsigned long arch_local_save_flags(void)
{
	unsigned long cp;

    __asm__(
            "mrs %0, cpsr\n"
            :"=r" (cp)
           );
    
    return cp;
}

inline void arch_local_irq_restore(unsigned int flags)
{
	 __asm__(
            "msr cpsr_cxsf, %0\n"
            :
            :"r" (flags)
           );
}



void umask_int(unsigned int offset)
{
	INTMSK &= ~(1 << offset);
}

void usubmask_int(unsigned int offset)
{
    INTSUBMSK &= ~(1 << offset);
}

void common_irq_handler()
{
	int bit;
    unsigned long oft = INTOFFSET;

    SRCPND = (1 << oft);
    INTPND |= (1 << oft);

	//~{GeVP6O~}
    if (oft == IRQ_EINT4t7 || oft == IRQ_EINT8t23) 
    {
        for (bit = 4; bit < 24; bit++)
        {
        	if (EINTPEND & (1 << bit))
        	{
        		oft = bit;
        		break;
        	}
        }
        EINTPEND |= (1 << bit);   // EINT4_7~{:OSC~}IRQ4

        oft += EXTINT_OFFSET;
    }

    deliver_irq(oft);
}

void s3c24xx_irq_mask(unsigned int irq_num)
{

    if (irq_num <= IRQ_ADC)
	    INTMSK |= (1 << irq_num);
}

void s3c24xx_irq_unmask(unsigned int irq_num)
{
    if (irq_num <= IRQ_ADC)
	    INTMSK &= ~(1 << irq_num);
}

void s3c24xx_irq_set_flag(unsigned irq_num, unsigned int flag)
{

}

void s3c24xx_irqext_mask(unsigned int irq_num)
{
    if (irq_num <= IRQ_EINT3)
        INTMSK |= (1 << irq_num);
    else if ((irq_num >= IRQ_EINT4) && (irq_num <= IRQ_EINT23))
        EINTMASK |= (1 << (irq_num - EXTINT_OFFSET));
}

void s3c24xx_irqext_unmask(unsigned int irq_num)
{
    if (irq_num <= IRQ_EINT3) 
        INTMSK &= ~(1 << irq_num);
    else if ((irq_num >= IRQ_EINT4) && (irq_num <= IRQ_EINT7))
    {   
        INTMSK   &= ~(1 << 4);
        EINTMASK &= ~(1 << (irq_num - EXTINT_OFFSET));
    }
    else if ((irq_num >= IRQ_EINT4) && (irq_num <= IRQ_EINT7))
    {   
        INTMSK   &= ~(1 << 5);
        EINTMASK &= ~(1 << (irq_num - EXTINT_OFFSET));
    }
    else
    {

    }
}

void s3c24xx_irqext_set_flag(unsigned irq_num, unsigned int flag)
{
    if (irq_num < IRQ_EINT4)
        s3c24xx_irq_set_flag(irq_num, flag);

    if ((irq_num >= IRQ_EINT4) && (irq_num <= IRQ_EINT23))
    {
        switch (flag)
        {
        case IRQ_FLAG_LOW_LEVEL:
            break;
        case IRQ_FLAG_HIGH_LEVEL:
            break;
        case IRQ_FLAG_FALLING_EDGE:
            break;
        case IRQ_FLAG_RISING_EDGE:
            break;
        case IRQ_FLAG_BOTH_EDGE:
            break;
        default:
            break;
        }
    }
}

void s3c24xx_init_irq(void)
{
    unsigned int irq_num;
    struct irq_desc *desc;

    for (irq_num = IRQ_EINT0; irq_num <= IRQ_EINT23; irq_num++)
    {
        desc = kmalloc(sizeof (struct irq_desc));
        if (!desc)
        {
            printk("%s failed!\n", __func__);
            panic();
        }
        desc->irq_num   = irq_num;
        if ((irq_num <= IRQ_EINT3) || (irq_num >= IRQ_EINT4 && irq_num <= IRQ_EINT23))
        {
            desc->mask      = s3c24xx_irqext_mask;
            desc->unmask    = s3c24xx_irqext_unmask;
            desc->set_flag  = s3c24xx_irqext_set_flag;
        }
        /*
         * else if (irq has sub_irq)
         * {
         * xxxxxxxxxxxxxxxxxxxxxxxxxxxx
         * }
         * */
        else
        {
            desc->mask      = s3c24xx_irq_mask;
            desc->unmask    = s3c24xx_irq_unmask;
            desc->set_flag  = s3c24xx_irq_set_flag;
        }

        if (register_irq_desc(desc) < 0)
        {
            printk("register_irq_desc failed! \n");
            panic();
        }

    }
}


