#include "sched.h" 
#include "prosser.h"
#include "sched.h"
#include "timer.h"
#include "pgtable.h"
#include "common.h"
#include "error.h"
#include "slab.h"
#include "mm.h"
 

extern void tmp_add_task_struct(struct task_struct *tsk);
extern struct task_struct *test_task_struct[2];
void wake_up_process(struct task_struct * p)
{
    test_task_struct[1] = p;
}

void switch_current_mm(void)
{
    if (old_task && old_task->mm == current->mm)
        return;

    if(current->mm)
        do_switch_mm(current->mm);
}