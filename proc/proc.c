#include "mm.h"
#include "arch.h"
#include "kernel.h"
#include "interrupt.h"
#include "error.h"
#include "config.h"
#include "sched.h"
#include "proc.h"
#include "wait.h"
#include "timer.h"
#include "lib.h"
#include "printk.h"
#include "vfs.h"
#include "syscall.h"
#include "completion.h"

extern void pcb_list_add(struct task_struct *head, struct task_struct *pcb);
extern struct task_struct * pcb_list_init(void);
extern void __soft_schedule(void);
extern void __int_schedule(void);
extern struct task_struct *alloc_pcb(void);
extern void *alloc_stack(void);

struct task_struct *next_run = NULL;
struct task_struct *current = NULL;
struct task_struct *old_task = NULL;
static struct proc_list_head proc_list[PROCESS_PRIO_BUTT];
static struct task_struct sleep_proc_list_head;
static unsigned int OS_TICKS = 0;
static unsigned int pid = 0;
unsigned char OS_RUNNING = 0;
extern unsigned int OSIntNesting;

#define DO_INIT_SP(sp,fn,args,lr,cpsr,pt_base)									\
		do{																		\
				(sp)=(sp)-4;/*r15*/												\
				*(volatile unsigned int *)(sp)=(unsigned int)(fn);/*r15*/		\
				(sp)=(sp)-4;/*cpsr*/											\
				*(volatile unsigned int *)(sp)=(unsigned int)(cpsr);/*r14*/		\
				(sp)=(sp)-4;/*lr*/												\
				*(volatile unsigned int *)(sp)=(unsigned int)(lr);				\
				(sp)=(sp)-4*13;/*r12,r11,r10,r9,r8,r7,r6,r5,r4,r3,r2,r1,r0*/	\
				*(volatile unsigned int *)(sp)=(unsigned int)(args);			\
		}while(0)

void DO_INIT_CONTEXT(struct pt_regs *regs, unsigned long fn, void *args, unsigned long lr, unsigned long cpsr, unsigned long sp, unsigned long pt_base)
{
    regs->ARM_pc = fn;
    regs->ARM_cpsr = cpsr;
    regs->ARM_lr = lr;
    regs->ARM_sp = sp;
    regs->ARM_r0 = 0xa5;
    regs->ARM_r1 = 1;
    regs->ARM_r2 = 2;
    regs->ARM_r3 = 3;
    regs->ARM_r4 = 4;
    regs->ARM_r5 = 5;
    regs->ARM_r6 = 6;
    regs->ARM_r7 = 7;
    regs->ARM_r8 = 8;
    regs->ARM_r9 = 9;
    regs->ARM_r10 = 10;
    regs->ARM_fp = 11;
    regs->ARM_ip = 12;
}

void *get_process_sp()
{
	void *tmp;
    
    tmp = (void *)alloc_stack();
    printk("get_sp_addr: %x\r\n", (unsigned int)tmp);
	return tmp;
}

unsigned int get_cpsr(void)
{
		unsigned long p;
	asm volatile
    (
		"mrs %0,cpsr\n"
		:"=r"(p)
		:
	);
	return p;
}

void thread_exit(void)
{
    while (1)
    {
        ssleep(1);
    }
}

int kernel_thread(int (*f)(void *), void *args)
{ 
    unsigned long flags;
	unsigned int sp;
	
	struct task_struct *pcb = (struct task_struct *)alloc_pcb();
	if (!pcb)
		return -ENOMEM;

	if((sp = (unsigned int)get_process_sp()) == 0)
	{
		printk("kernel_thread get sp_space error\r\n");
		return -ENOMEM;
	}

	printk("get_pcb_addr: %x\r\n", (unsigned int)pcb);

	sp 				        = (sp + TASK_STACK_SIZE);
	pcb->sp 		        = sp;
//	pcb->sp_bottom          = sp;
	pcb->sp_size 	        = TASK_STACK_SIZE;

    pcb->preempt_count      = 0;

    pcb->prio               = PROCESS_PRIO_NORMAL;
	pcb->pid 		        = pid++;
	pcb->time_slice         = 5;
	pcb->ticks 		        = 5;
	pcb->root 		        = current->root;
	pcb->pwd  		        = current->pwd;
	memcpy(pcb->filp, current->filp, sizeof (pcb->filp));
	
	DO_INIT_CONTEXT(&pcb->regs, f, args, thread_exit, 0x1f & get_cpsr(), pcb->sp, 0);

    pcb->mm = current->mm;

//	enter_critical();
    
    local_irq_save(flags);

    pcb_list_add(&proc_list[pcb->prio].head, pcb);
    
    local_irq_restore(flags);
//  exit_critical();

	return 0;
}

int kernel_thread_prio(int (*f)(void *), void *args, unsigned int prio)
{
	unsigned int sp;
	
	struct task_struct *pcb = (struct task_struct *)alloc_pcb();
	if (!pcb)
		return -ENOMEM;

	if((sp = (unsigned int)get_process_sp()) == 0)
	{
		printk("kernel_thread get sp_space error\r\n");
		return -ENOMEM;
	}

	printk("get_pcb_addr: %x\r\n", (unsigned int)pcb);

	sp 				= (sp + TASK_STACK_SIZE);
	pcb->sp 		= sp;
//	pcb->sp_bottom  = sp;
	pcb->sp_size 	= TASK_STACK_SIZE;

    pcb->prio       = prio;
	pcb->pid 		= pid++;
	pcb->time_slice = 5;
	pcb->ticks 		= 5;
	pcb->root 		= current->root;
	pcb->pwd  		= current->pwd;
	memcpy(pcb->filp, current->filp, sizeof (pcb->filp));
	
	DO_INIT_CONTEXT(&pcb->regs, f, args, thread_exit, 0x1f & get_cpsr(), pcb->sp, 0);

    pcb->mm = current->mm;
	enter_critical();
	pcb_list_add(&proc_list[prio].head, pcb);
	exit_critical();

	return 0;
}

struct task_struct *tmp_test_create_thread(int (*f)(void *), void *args)
{ 
    unsigned long flags;
	unsigned long sp;
	
	struct task_struct *pcb = (struct task_struct *)alloc_pcb();
	if (!pcb)
		return NULL;

	if((sp = (unsigned long)get_process_sp()) == 0)
	{
		printk("kernel_thread get sp_space error\r\n");
		return NULL;
	}

	printk("get_pcb_addr: %x\r\n", (unsigned int)pcb);

	sp 				        = (sp + TASK_STACK_SIZE);
	pcb->sp 		        = sp;
//	pcb->sp_bottom          = sp;
	pcb->sp_size 	        = TASK_STACK_SIZE;

    pcb->preempt_count      = 0;

    pcb->prio               = PROCESS_PRIO_NORMAL;
	pcb->pid 		        = pid++;
	pcb->time_slice         = 5;
	pcb->ticks 		        = 5;
	//pcb->root 		        = current->root;
	//pcb->pwd  		        = current->pwd;
	//memcpy(pcb->filp, current->filp, sizeof (pcb->filp));
    
    if (current)
        sp = current->current_max_sp + TASK_STACK_SIZE;
    else 
        sp = 0x80000000;
    
    current = pcb;
    pcb->mm = test_switch_mm_alloc_mm();
    memcpy((void *)pcb->mm->pgd, (void *)TLB_BASE, 4096 * 4);

    do_brk(sp, TASK_STACK_SIZE);
    pcb->sp = sp + TASK_STACK_SIZE;
    pcb->sp_bottom = sp;

    current->current_max_sp = sp;
   
	
	DO_INIT_CONTEXT(&pcb->regs, f, args, thread_exit, 0x1f & get_cpsr(), pcb->sp, 0);
	

    return pcb;
}

extern struct inode *root_inode;
int create_init_thread(int (*f)(void *), void *args)
{ 
    unsigned long flags;
	unsigned long sp;
	
	struct task_struct *pcb = (struct task_struct *)alloc_pcb();
	if (!pcb)
		return NULL;
    
    current = pcb;

    pcb->preempt_count      = 0;

    pcb->prio               = PROCESS_PRIO_NORMAL;
	pcb->pid 		        = 0;
	pcb->time_slice         = 5;
	pcb->ticks 		        = 5;
    
    sp = 0x80000000;
    
    pcb->mm = test_switch_mm_alloc_mm();
    memcpy((void *)pcb->mm->pgd, (void *)TLB_BASE, 4096 * 4);

    do_brk(sp, TASK_STACK_SIZE);
    pcb->sp = sp + TASK_STACK_SIZE;
    pcb->sp_bottom = sp;

    current->current_max_sp = sp;
	DO_INIT_CONTEXT(&pcb->regs, f, args, thread_exit, 0x1f & get_cpsr(), pcb->sp, 0);
    
    pcb_list_add(&proc_list[pcb->prio].head, pcb);
    

    return 0;
}

struct task_struct *OS_GetNextReady(void)
{
    int prio;
    struct task_struct *tmp;

    //return tmp_get_next_ready();
    
    for (prio = 0; prio < PROCESS_PRIO_BUTT; prio++)
    {
        tmp = proc_list[prio].current;
        
        do
        {
            tmp = tmp->next;
            if (tmp->pid == -1)
                continue;
            if (tmp->flags == PROCESS_READY)
            {
                proc_list[prio].current = tmp;
                old_task = current;
                return tmp;
            }
        } while (tmp != proc_list[prio].current);
    }

    enter_critical();
    printk("No Ready Process!! \n");
    panic();

    return tmp;
}

void OS_IntSched()
{
	//if (preempt_count > 0)
    //    return;

	current = OS_GetNextReady();
    
	__int_schedule();
}

void OS_Sched()
{

    if (!OS_RUNNING)
    {
        printk("OS_Sched Before OS_RUNNING\n");
        panic();
    }

    //if (preempt_count > 0)
    //    return;

    kernel_disable_irq(); 
    
    next_run = OS_GetNextReady();
	
	__soft_schedule();
    
    kernel_enable_irq();
}

void schedule(void)
{
    if (OSIntNesting > 0)
        OS_IntSched(); 
    else
        OS_Sched();
}

void __wake_up(wait_queue_t *wq)
{

}

void prepare_to_wait(wait_queue_t *wq, unsigned int state)
{
    wq->priv = current;
    current->flags |= state;
}

void finish_wait(wait_queue_t *wq)
{
    struct task_struct *pcb;

    if (!wq || !wq->priv)
        return;

    pcb = wq->priv;
    pcb->flags = PROCESS_READY;
}

void __wake_up_interruptible(wait_queue_t *wq)
{

    struct task_struct *pcb;

    if (!wq || !wq->priv)
        return;

    pcb = wq->priv;
    pcb->flags = PROCESS_READY;
}

void __init_waitqueue_head(wait_queue_t *wq, char *name)
{
    spin_lock_init(&wq->lock);
    INIT_LIST_HEAD(&wq->task_list);
}


void process_timeout(void *data)
{
    set_task_state(data, PROCESS_READY);
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *定时器在栈上,如果进程异常退出没有清掉定时器就会引发异常
 *
 * 只验证了 MAX_SCHEDULE_TIMEOUT 分支,未验证超时分支
 */
long schedule_timeout(long timeout, unsigned int state)
{
    unsigned long expire;
    struct timer_list timer;

    switch (timeout)
    {
    case MAX_SCHEDULE_TIMEOUT:
        schedule();
        return timeout < 0 ? 0 : timeout;
    default:
        break;
    }
    
    expire = OS_TICKS + timeout;

    timer.function = process_timeout;
    timer.data = current;
    timer.expires = timeout;

    add_timer(&timer);
    schedule();
    del_timer(&timer);

    timeout = expire - OS_TICKS;
    
    return timeout < 0 ? 0 : timeout;
}

long do_wait_for_common(struct completion *x, long timeout, unsigned int state)
{
    wait_queue_t wq;

    if (!x->done)
    {
        init_waitqueue_head(&wq);
        wq.priv = current;
        list_add_tail(&wq.task_list, &x->wq.task_list);
        
        do
        {
            set_current_state(state);
            spin_unlock_irq(&x->wq.lock);
            timeout = schedule_timeout(timeout, state);
            spin_lock_irq(&x->wq.lock);
        } while (!x->done && timeout);

        if (!x->done)
            return timeout;
    }

    x->done--;
    return timeout ? 1 : 0;
}

long wait_for_common(struct completion *x, long timeout, int state)
{
    spin_lock_irq(&x->wq.lock);
    
    timeout = do_wait_for_common(x, timeout, state);
    
    spin_unlock_irq(&x->wq.lock);

    return timeout; 
}

void wait_for_completion(struct completion *x)
{
    wait_for_common(x, MAX_SCHEDULE_TIMEOUT, PROCESS_WAIT);
}

void complete(struct completion *x)
{
	unsigned long flags;
    wait_queue_t *wq;
    struct list_head *list;

    spin_lock_irqsave(&x->wq.lock, flags);  
   
    x->done++;
   
    if (!list_empty(&x->wq.task_list))
    {
        list = x->wq.task_list.next;
        wq = list_entry(list, wait_queue_t, task_list);
        list_del(list);

        set_task_state(wq->priv, PROCESS_READY);
    }
    spin_unlock_irqrestore(&x->wq.lock, flags);
}

void init_completion(struct completion *x)
{
    x->done = 0;
    init_waitqueue_head(&x->wq);
}

int OS_SYS_PROCESS(void *p)
{
#if 0
	int src;
	while (1)
    {
        MESSAGE msg;
        while (1) 
        {
            send_recv(RECEIVE, ANY, &msg);
            src = msg.source;

            switch (msg.type) {
            case GET_TICKS:
                msg.RETVAL = 666;
                send_recv(SEND, src, &msg);
                break;
            default:
   //             panic("unknown msg type");
                break;
            }
        }
	}
	//return 0;
    //
#else
    while (1)
        ssleep(100);
#endif
}

int OS_IDLE_PROCESS(void *arg)
{
	while (1)
	{
		printk("OS Idle Process\r\n");
		OS_Sched();
	}
}

void print_sleep_list(void)
{
    struct task_struct *tmp;

    tmp = sleep_proc_list_head.next_sleep_proc;

    while (tmp != &sleep_proc_list_head)
    {
        printk("pid: %d sleep_time: %d\n", tmp->pid, tmp->sleep_time);
        tmp = tmp->next_sleep_proc;
    }
}

void add_current_to_sleep_list(void)
{
   sleep_proc_list_head.next_sleep_proc->prev_sleep_proc = current;
   
   current->next_sleep_proc = sleep_proc_list_head.next_sleep_proc;
   current->prev_sleep_proc = &sleep_proc_list_head;
  
   sleep_proc_list_head.next_sleep_proc = current;
}

void process_sleep(unsigned int sec)
{
	enter_critical();

	current->flags |= PROCESS_SLEEP;
	current->sleep_time = sec * HZ;

    add_current_to_sleep_list();

	exit_critical();
	OS_Sched();
}

void process_msleep(unsigned int m)
{
	enter_critical();

	current->flags |= PROCESS_SLEEP;
	current->sleep_time = m * HZ / 1000;
	if (!current->sleep_time)
		current->sleep_time = 1;
    
    add_current_to_sleep_list();

	exit_critical();
	OS_Sched();
}

void set_task_state(void *task, unsigned int state)
{
    struct task_struct *tsk = task;

	enter_critical();

	tsk->flags = state;

	exit_critical();
}

void set_current_state(unsigned int state)
{
    set_task_state(current, state);
}

void clr_task_status(unsigned int status)
{
	enter_critical();

	current->flags &= ~(status);

	exit_critical();
}

void update_sleeping_proc(void)
{
    struct task_struct *tmp;
	
    tmp = sleep_proc_list_head.next_sleep_proc;
	while (tmp != &sleep_proc_list_head)
	{
        if (tmp->sleep_time > 0)
            tmp->sleep_time--;
        if (tmp->sleep_time == 0)
        {
            tmp->flags &= ~(PROCESS_SLEEP);
            tmp->next_sleep_proc->prev_sleep_proc = tmp->prev_sleep_proc; 
            tmp->prev_sleep_proc->next_sleep_proc = tmp->next_sleep_proc;
        }
        tmp = tmp->next_sleep_proc;
	}

}

void OS_Clock_Tick(void *arg)
{
    //
    OS_TICKS++;
    //

    timer_list_process();
    update_sleeping_proc();

    if (current->ticks > 0)	
        current->ticks--;
   
    if (current->ticks > 0)
        return;

    current->ticks = current->time_slice;
    
    OSIntNesting--;
    OS_IntSched();

    /*OSIntNesting-- 不能放在这里，因为代码不会执行到这儿 */
}

unsigned int OS_Get_Kernel_Ticks(void)
{
    return OS_TICKS;
}
		
void panic(void)
{
	while (1)
		;
}

extern int test_user_syscall_open(void *argc);
extern int  test_nand(void *p);
extern int  test_get_ticks(void *p);
extern int  test_open_led0(void *p);
extern int  test_open_led1(void *p);
extern int  test_open_led2(void *p);
extern int  test_open_led3(void *p);
extern int test_user_syscall_printf(void *argc);
extern int test_wait_queue(void *p);
extern int test_socket(void *p);
extern int led_driver_init(void);
extern int led_device_init(void);
extern void LWIP_INIT(void);
extern int socket_init(void);
extern int create_stdin_stdout_stderr_device(void);
extern void bus_list_init(void);
extern int platform_bus_init(void);
extern int test_exit(void *arg);
extern int test_completion(void *arg);
extern int spi_module_init(void);
extern int test_oled(void *arg);
extern int test_oled1(void *arg);
extern int dm9000_module_init(void);
extern void s3c24xx_controler_init(void);
extern void spi_info_jz2440_init(void);
extern int spi_oled_init(void);
extern void sys_timer_init(void);
extern int test_fork(void *arg);
extern int test_single_mm(void *arg);
extern int test_brk(void *arg);

int OS_INIT_PROCESS(void *argv)
{
	int ret;
	int fd_stdin;
	int fd_stdout;
	int fd_stderr;

	enter_critical();
	sys_timer_init();

	fd_stdout = sys_open("/dev/stdout", 0, 0);
	if (fd_stdout < 0)
	{
		printk("Init Process Open stdout device failed\n");
		panic();
	}

	fd_stdin = sys_open("/dev/stdin", 0, 0);
	if (fd_stdin < 0)
	{
		printk("Init Process Open stdin device failed\n");
		panic();
	}

	fd_stderr = sys_open("/dev/stderr", 0, 0);
	if (fd_stderr < 0)
	{
		printk("Init Process Open stderr device failed\n");
		panic();
	}

#if 1

#if 0
	ret = kernel_thread(OS_SYS_PROCESS,  (void *)0);
	if (ret < 0)
	{
		printk("create OS_SYS_PROCESS error\n");
		panic();
	}
#endif

	ret = kernel_thread_prio(OS_IDLE_PROCESS, (void *)0, PROCESS_PRIO_IDLE);
	if (ret < 0)
	{
		printk("create OS_IDLE_PROCESS error\n");
		panic();
	}
	
	//led_module_init();
	led_driver_init();
	led_device_init();
	//key_module_init();

	socket_init();
	net_core_init();
	dm9000_module_init();

    spi_module_init();
    s3c24xx_controler_init();
    spi_info_jz2440_init();
    spi_oled_init();

	//create_pthread(test_get_ticks, (void *)1, 10);
	kernel_thread(test_open_led0, (void *)2);
	kernel_thread(test_open_led1, (void *)2);
	kernel_thread(test_open_led2, (void *)2);
	kernel_thread(test_open_led3, (void *)2);
//	kernel_thread(test_nand, (void *)2);
//	kernel_thread(test_user_syscall_open, (void *)2);
	kernel_thread_prio(test_user_syscall_printf, (void *)2, PROCESS_PRIO_HIGH);
//	kernel_thread(test_wait_queue, (void *)2);
	//kernel_thread(test_socket, (void *)2);
	kernel_thread(test_completion, (void *)2);
	kernel_thread(test_oled, (void *)2);
//	kernel_thread(test_oled1, (void *)2);
//	kernel_thread_prio(test_exit,   (void *)2, PROCESS_PRIO_LOW);
    
    OS_RUNNING = 1;
    exit_critical();

#endif
	while (1)
	{
		//printk("OS Init Process\r\n");
		OS_Sched();
	}
    
    //return 0;
}

int OS_Init(void)
{
    int prio;
	int ret;

    sleep_proc_list_head.pid = -1;
    sleep_proc_list_head.next_sleep_proc = &sleep_proc_list_head;
    sleep_proc_list_head.prev_sleep_proc = &sleep_proc_list_head;

    for (prio = 0; prio < PROCESS_PRIO_BUTT; prio++)
    {
        proc_list[prio].head.pid = -1;
        proc_list[prio].head.next = &proc_list[prio].head;
        proc_list[prio].head.prev = &proc_list[prio].head;
        proc_list[prio].current = &proc_list[prio].head;
    }
	
	ret = system_mm_init();
	if (ret < 0)
	{
		printk("system kmalloc init error\n");
		panic();
	}
	
    s3c24xx_init_irq();
	s3c24xx_init_tty();
	
    ret = create_init_thread(OS_INIT_PROCESS, (void *)0);
	if (ret < 0)
	{
		printk("create OS_INIT_PROCESS error\n");
		panic();
	}
	
    ret = vfs_init();
	if (ret < 0)
	{
		printk("vfs init error\n");
		panic();
	}

	ret = sys_mkdir("/dev", 0);
	if (ret < 0)
	{
		printk("/dev directory error\n");
		panic();
	}
	
	ret = create_stdin_stdout_stderr_device();
	if (ret < 0)
	{
		printk("create_std_device error\n");
		panic();
	}

	bus_list_init();
    ret = platform_bus_init();
    if (ret < 0)
    {
    	printk("platform bus init failed");
    	panic();
    }

	
	current = proc_list[PROCESS_PRIO_NORMAL].head.next;

	timer_list_init();
    
    return 0;
}

int proc2pid(struct task_struct *proc)
{
#if 0
    return proc->pid;
#else
    enter_critical();
    
    printk("Uncomplete Function %s\n", __func__);
   
    panic();

    return 0;
#endif
}

struct task_struct *pid2proc(int pid)
{

    enter_critical();
    
    printk("Uncomplete Function %s\n", __func__);
   
    panic();

    return 0;
}

