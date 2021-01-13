#ifndef __PCB_H__
#define __PCB_H__

#include "message.h"
#include "fs.h"
#include "config.h"
#include "wait.h"
#include "mm.h"
#include "ptrace.h"
#include "printk.h"

#define disable_schedule(x)	enter_critical()
#define enable_schedule(x)	exit_critical()

typedef enum 
{
	running,
	ready,
	wait,
	sleep,
}status;

typedef struct _heap_slab
{
	unsigned int heap_slab_start;
	unsigned int heap_slab_size;
	struct _heap_slab *heap_slab_next;
}heap_slab;

struct task_struct
{
	struct pt_regs	regs;
	unsigned int sp;
	unsigned int sp_size;
	unsigned int sp_bottom;
    
	int did_exec;

	int pid;
	char proc_name[16];
	unsigned int flags;
	unsigned int sleep_time;

	unsigned int state;

	struct mm_struct *mm;
	struct mm_struct *active_mm;

    volatile unsigned long need_resched;

    unsigned long counter;
    int lock_depth;
    unsigned long start_time;

    struct list_head run_list;

	unsigned int prio;

    unsigned int preempt_count;

	unsigned long process_mem_size;
	unsigned long phyaddr;
	unsigned long viraddr;

	unsigned int authority;//锟斤拷锟斤拷权锟睫ｏ拷锟节核斤拷锟教★拷锟矫伙拷锟斤拷锟教ｏ拷

	int time_slice;
	int ticks;

	wait_queue_t wq;
	wait_queue_t wait_childexit;
	spinlock_t alloc_lock;

	MESSAGE *p_msg;
	int p_recvfrom;
	int p_sendto;
	struct task_struct *q_sending;
	struct task_struct *next_sending;
	int has_int_msg;           /**
									* nonzero if an INTERRUPT occurred when
									* the task is not ready to deal with it.
									*/
	int nr_tty;
	
	struct task_struct *p_opptr, *p_pptr, *p_cptr, *p_ysptr, *p_osptr;

	unsigned long min_flt, maj_flt, nswap, cmin_flt, cmaj_flt, cnswap;

	struct task_struct 		*next;
	struct task_struct 		*prev;
	struct file 	*filp[NR_OPEN];
	struct inode	*pwd;
	struct inode 	*root;

    struct task_struct      *next_sleep_proc;
    struct task_struct      *prev_sleep_proc;
};

struct proc_list_head
{
    struct task_struct head;
    struct task_struct *current;
    unsigned int prio;
    unsigned int proc_cnt;
};

#endif 

