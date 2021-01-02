#include "sched.h" 
#include "prosser.h"
#include "sched.h"
#include "timer.h"
#include "pgtable.h"
#include "common.h"
#include "error.h"
#include "slab.h"
#include "mm.h"

extern void cpu_arm920_set_pgd(unsigned long pgd);
 
void wake_up_process(struct task_struct * p)
{

}

void switch_mm(struct mm_struct *mm)
{
    cpu_arm920_set_pgd((unsigned long )mm->pgd);
}