#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#define MAX_IRQ_NUMBER      64

typedef void (* irq_server)(void *);

typedef struct
{
    irq_server	 irq_handler;
    unsigned int irq_num;
    void 		 *priv;
}irq_handler;

extern int put_irq_handler(unsigned int , irq_server, void *);
extern void enable_irq(void);
extern void disable_irq(void);

#endif

