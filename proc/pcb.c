#include "sched.h"
#include "config.h"
#include "list.h"
#include "printk.h"

typedef struct 
{
	unsigned char sp[TASK_STACK_SIZE];
}stack_t;


struct task_struct 	task_pcb[MAX_TASK_NUM];
stack_t	task_sp[MAX_TASK_NUM];
struct task_struct *alloc_pcb(void)
{
	static unsigned int cur_pcb = 0;
	
	if (cur_pcb < MAX_TASK_NUM)
	{
		INIT_LIST_HEAD(&task_pcb[cur_pcb].wq.task_list);
		return &task_pcb[cur_pcb++];
	}

	return 0;
}

void *alloc_stack(void)
{

	static unsigned int cur_sp = 0;
	if (cur_sp < MAX_TASK_NUM)
		return (void *)(task_sp[cur_sp++].sp);

	return 0;
}


void pcb_list_add(struct task_struct *head, struct task_struct *pcb)
{
	struct task_struct *tmp = pcb;

    tmp->next = head->next;
    tmp->prev = head;
    head->next->prev = tmp;
    head->next = tmp;
}

struct task_struct *pcb_list_init(void)
{
	struct task_struct *tmp;

    if ((tmp = (struct task_struct *)alloc_pcb()) == (void *)0)
    {
        printk("%s failed\n", __func__);
        panic();
    }

	tmp->next = tmp;
	tmp->prev = tmp;
	tmp->pid = -1;

	return tmp;
}

