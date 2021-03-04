#ifndef __KERNEL_FORK_H__
#define __KERNEL_FORK_H__

#include "ptrace.h"

struct mm_struct *mm_alloc(void);

extern int sys_fork(struct pt_regs *reg);
extern void proc_caches_init(void);
extern int do_fork(unsigned long clone_flags, unsigned long stack_start, struct pt_regs *regs, unsigned long stack_size);


#endif

