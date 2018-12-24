#include "kernel.h"
#include "error.h"
#include "interrupt.h"
#include "common.h"
#include "kmalloc.h"
#include "printk.h"

static struct irq_desc *irq_desc_table[MAX_IRQ_NUMBER];
unsigned int OSIntNesting = 0;

/* 暂不支持中断嵌套, OSIntNesting == 1 表示当前程序处理中断当中, OSIntNesting = 0 表示未处理中断当中*/
static unsigned int irq_lock = 0;
void kernel_disable_irq()
{
    if (OSIntNesting > 0) /* 内核不支持中断嵌套, OSIntNesting = 1的时候表示已经处于中断当中, 这时中断是关闭的 */
        return;
    
    irq_lock++;
    disable_irq();
}

void kernel_enable_irq()
{
    if (OSIntNesting > 0) /*   */
        return;

    irq_lock--;
    if (irq_lock)
        return;
    enable_irq();
}

void enter_critical()
{

}


void exit_critical()
{

}

void deliver_irq(int irq_num)
{
    struct irq_desc *desc;

    if (irq_num >= MAX_IRQ_NUMBER)
        return;

    desc = irq_desc_table[irq_num];
    
    if (desc && desc->irq_handler)
    {
        OSIntNesting++;
    
        desc->count++;
        desc->irq_handler(desc->priv);
        
        OSIntNesting--;
    }
}

int register_irq_desc(struct irq_desc *desc)
{
    if (!desc)
        return -EINVAL;

    if (desc->irq_num >= MAX_IRQ_NUMBER)
        return -ENOMEM;

    if (irq_desc_table[desc->irq_num])
        return -EBUSY;

    irq_desc_table[desc->irq_num] = desc;

    return 0;
}

void unregister_irq_desc(unsigned int irq_num)
{
    struct irq_desc *desc;
    
    if (irq_num >= MAX_IRQ_NUMBER)
        return;
    desc = irq_desc_table[irq_num];

    kfree(desc);
    irq_desc_table[desc->irq_num] = NULL;
}

int request_irq(int irq_num, irq_server irq_handler, unsigned int flag, void *priv)
{
    struct irq_desc *desc;
    
    if (!irq_handler)
        return -EINVAL;

    if (irq_num >= MAX_IRQ_NUMBER)
        return -ENOMEM;
   
    desc = irq_desc_table[irq_num];
    if (desc->irq_handler) 
        return -EBUSY;
    
    desc->priv = priv;
    desc->irq_handler = irq_handler;
    desc->set_flag(irq_num, flag);
    desc->unmask(irq_num);

    return 0;
}

void free_irq(int irq_num)
{
    struct irq_desc *desc;

    if (irq_num >= MAX_IRQ_NUMBER)
        return;
    
    desc = irq_desc_table[irq_num];
    if (!desc)
        return;

    
    desc->mask(irq_num);
    
    desc->irq_handler = NULL;
    desc->flag = 0;
    desc->count = 0;

    /*
     * wakt_up(all wait for threads)
     * */
}


