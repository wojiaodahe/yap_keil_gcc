#include "prosser.h"
#include "lib.h"

#define ll_alloc_task_struct() ((struct task_struct *)__get_free_pages(GFP_KERNEL, 1))
#define ll_free_task_struct(p) free_pages((unsigned long)(p), 1)

int copy_thread(int nr, unsigned long clone_flags, unsigned long esp, unsigned long unused, struct task_struct *p, struct pt_regs *regs)
{
    memcpy(&p->regs, regs, sizeof (struct pt_regs));
    p->regs.ARM_r0 = 0;
    return 0;
}

struct task_struct *alloc_task_struct(void)
{
    struct task_struct *tsk;

    tsk = ll_alloc_task_struct();

    return tsk;
}

void __free_task_struct(struct task_struct *p)
{
    ll_free_task_struct(p);
}

void free_task_struct(struct task_struct *p)
{
    __free_task_struct((p));
}