#ifndef __ARM_PROSSER_H__
#define __ARM_PROSSER_H__

#include "arm/ptrace.h"
#include "sched.h"
#include "timer.h"
#include "pgtable.h"
#include "common.h"
#include "error.h"
#include "slab.h"
#include "mm.h"

struct task_struct *alloc_task_struct(void);
void free_task_struct(struct task_struct *p);
int copy_thread(int nr, unsigned long clone_flags, unsigned long esp, unsigned long unused, struct task_struct *p, struct pt_regs *regs);
void do_switch_mm(struct mm_struct *mm);

#endif