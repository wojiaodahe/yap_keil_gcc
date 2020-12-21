#ifndef __KERNEL_TIMER_H__
#define __KERNEL_TIMER_H__


#include "list.h"

extern unsigned long volatile jiffies;

struct timer_list
{
	unsigned long expires;
	void *data;
	void (*function)(void *);
	struct list_head list;
};

extern void timer_list_process(void);
extern int mod_timer(struct timer_list *timer, unsigned long expires);
extern int add_timer(struct timer_list *timer);
extern void del_timer(struct timer_list *timer);
extern void timer_list_init(void);

#endif /* INCLUDE_TIMER_H_ */
