#ifndef __KERNEL_SCHED_H__
#define __KERNEL_SCHED_H__

#include "mm.h"
#include "spin_lock.h"
#include "atomic.h"
#include "slab.h"
#include "fs.h"

#define NR_OPEN_DEFAULT 32

#define TASK_RUNNING		0
#define TASK_INTERRUPTIBLE	1
#define TASK_UNINTERRUPTIBLE	2
#define TASK_ZOMBIE		4
#define TASK_STOPPED		8


struct task_struct 
{
    unsigned int pid;

    volatile long state; /* -1 unrunnable, 0 runnable, >0 stopped */
    unsigned long flags; /* per process flags, defined below */
    int did_exec;

    volatile long need_resched;

    long counter;
    int lock_depth;
    unsigned long start_time;

    struct mm_struct *mm;
    struct mm_struct *active_mm;

	struct task_struct *p_opptr, *p_pptr, *p_cptr, *p_ysptr, *p_osptr;

    struct list_head run_list;

    struct file *filp[NR_OPEN];
    struct inode *pwd;
    struct inode *root;

    unsigned long min_flt, maj_flt, nswap, cmin_flt, cmaj_flt, cnswap;
};

extern struct task_struct *current;
extern kmem_cache_t *sigact_cachep;
extern kmem_cache_t *files_cachep;
extern kmem_cache_t *fs_cachep;
extern kmem_cache_t *vm_area_cachep;
extern kmem_cache_t *mm_cachep;

/* Maximum number of active map areas.. This is a random (large) number */
#define MAX_MAP_COUNT	(65536)

/*
 * Per process flags
 */
#define PF_ALIGNWARN	0x00000001	/* Print alignment warning msgs */
					/* Not implemented yet, only for 486*/
#define PF_STARTING	0x00000002	/* being created */
#define PF_EXITING	0x00000004	/* getting shut down */
#define PF_FORKNOEXEC	0x00000040	/* forked but didn't exec */
#define PF_SUPERPRIV	0x00000100	/* used super-user privileges */
#define PF_DUMPCORE	0x00000200	/* dumped core */
#define PF_SIGNALED	0x00000400	/* killed by a signal */
#define PF_MEMALLOC	0x00000800	/* Allocating memory */
#define PF_VFORK	0x00001000	/* Wake up parent in mm_release */

#define PF_USEDFPU	0x00100000	/* task used FPU this quantum (SMP) */

/*
 * Ptrace flags
 */

#define PT_PTRACED	0x00000001
#define PT_TRACESYS	0x00000002
#define PT_DTRACE	0x00000004	/* delayed trace (used on m68k, i386) */

void wake_up_process(struct task_struct * p);

#endif
