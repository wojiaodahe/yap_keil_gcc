#include <stdio.h>
#include <string.h>

#include "ptrace.h"
#include "prosser.h"
#include "sched.h"
#include "timer.h"
#include "pgtable.h"
#include "common.h"
#include "error.h"
#include "slab.h"
#include "mm.h"
#include "fs.h"

/* SLAB cache for signal_struct structures (tsk->sig) */
kmem_cache_t *sigact_cachep;

/* SLAB cache for files_struct structures (tsk->files) */
kmem_cache_t *files_cachep;

/* SLAB cache for fs_struct structures (tsk->fs) */
kmem_cache_t *fs_cachep;

/* SLAB cache for vm_area_struct structures */
kmem_cache_t *vm_area_cachep;

/* SLAB cache for mm_struct structures (tsk->mm) */
kmem_cache_t *mm_cachep;

struct task_struct *current;


unsigned int nr_threads;
unsigned long total_forks;
spinlock_t mmlist_lock;


#define allocate_mm()	(kmem_cache_alloc(mm_cachep, SLAB_KERNEL))
#define free_mm(mm)	    (kmem_cache_free(mm_cachep, (mm)))


static struct mm_struct * mm_init(struct mm_struct * mm)
{
	atomic_set(&mm->mm_users, 1);
	atomic_set(&mm->mm_count, 1);
	//init_MUTEX(&mm->mmap_sem);
	//mm->page_table_lock = SPIN_LOCK_UNLOCKED;
	mm->pgd = pgd_alloc();
	if (mm->pgd)
		return mm;
	free_mm(mm);
	return NULL;
}
	

/*
 * Allocate and initialize an mm_struct.
 */
struct mm_struct *mm_alloc(void)
{
    struct mm_struct *mm;

    mm = allocate_mm();
    if (mm)
    {
        memset(mm, 0, sizeof(*mm));
        return mm_init(mm);
    }
    return NULL;
}

/*
 * Decrement the use count and release all resources for an mm.
 */
void mmput(struct mm_struct *mm)
{
#if 0
    if (atomic_dec_and_lock(&mm->mm_users, &mmlist_lock))
    {
        list_del(&mm->mmlist);
        spin_unlock(&mmlist_lock);
        exit_mmap(mm);
        mmdrop(mm);
    }
#endif

    printf("BUG: %s \n", __func__);
}

void proc_caches_init(void)
{
    pmd_t *pmd;
    pte_t *pte;
#if 0
    sigact_cachep = kmem_cache_create("signal_act",
                                      sizeof(struct signal_struct), 0,
                                      SLAB_HWCACHE_ALIGN, NULL, NULL);
    if (!sigact_cachep)
        panic("Cannot create signal action SLAB cache");

    files_cachep = kmem_cache_create("files_cache",
                                     sizeof(struct files_struct), 0,
                                     SLAB_HWCACHE_ALIGN, NULL, NULL);
    if (!files_cachep)
        panic("Cannot create files SLAB cache");

    fs_cachep = kmem_cache_create("fs_cache",
                                  sizeof(struct fs_struct), 0,
                                  SLAB_HWCACHE_ALIGN, NULL, NULL);
    if (!fs_cachep)
        panic("Cannot create fs_struct SLAB cache");

#endif
    vm_area_cachep = kmem_cache_create("vm_area_struct",
                                       sizeof(struct vm_area_struct), 0,
                                       SLAB_HWCACHE_ALIGN, NULL, NULL);
    if (!vm_area_cachep)
        printf("vma_init: Cannot alloc vm_area_struct SLAB cache");

    mm_cachep = kmem_cache_create("mm_struct",
                                  sizeof(struct mm_struct), 0,
                                  SLAB_HWCACHE_ALIGN, NULL, NULL);
    if (!mm_cachep)
        printf("vma_init: Cannot alloc mm_struct SLAB cache");

    init_mm.pgd = (pgd_t *)__get_free_pages(GFP_KERNEL, 0);
    pmd = pmd_alloc(init_mm.pgd, 0);
    pte = pte_alloc(pmd, 0);

    printf("init_mm.pgd: %p init_mm.pmd: %p init_mm.pte: %p\n", init_mm.pgd, pmd, pte);
}

int dup_mmap(struct mm_struct *mm)
{
#if FILE
    struct file *file;
#endif
    struct vm_area_struct *mpnt, *tmp, **pprev;
    int retval;

    //flush_cache_mm(current->mm);
    mm->locked_vm = 0;
    mm->mmap = NULL;
    mm->mmap_avl = NULL;
    mm->mmap_cache = NULL;
    mm->map_count = 0;
    //mm->cpu_vm_mask = 0;
    mm->swap_cnt = 0;
    mm->swap_address = 0;
    pprev = &mm->mmap;
    for (mpnt = current->mm->mmap; mpnt; mpnt = mpnt->vm_next)
    {

        retval = -ENOMEM;
        if (mpnt->vm_flags & VM_DONTCOPY)
            continue;
        tmp = kmem_cache_alloc(vm_area_cachep, SLAB_KERNEL);
        if (!tmp)
            goto fail_nomem;
        *tmp = *mpnt;
        tmp->vm_flags &= ~VM_LOCKED;
        tmp->vm_mm = mm;
        mm->map_count++;
        tmp->vm_next = NULL;

#if FILE
        file = tmp->vm_file;
        if (file)
        {
            struct inode *inode = file->f_dentry->d_inode;
            get_file(file);
            if (tmp->vm_flags & VM_DENYWRITE)
                atomic_dec(&inode->i_writecount);

            /* insert tmp into the share list, just after mpnt */
            spin_lock(&inode->i_mapping->i_shared_lock);
            if ((tmp->vm_next_share = mpnt->vm_next_share) != NULL)
                mpnt->vm_next_share->vm_pprev_share =
                    &tmp->vm_next_share;
            mpnt->vm_next_share = tmp;
            tmp->vm_pprev_share = &mpnt->vm_next_share;
            spin_unlock(&inode->i_mapping->i_shared_lock);
        }
#endif
        /* Copy the pages, but defer checking for errors */
        retval = copy_page_range(mm, current->mm, tmp);
        if (!retval && tmp->vm_ops && tmp->vm_ops->open)
            tmp->vm_ops->open(tmp);

        /*
		 * Link in the new vma even if an error occurred,
		 * so that exit_mmap() can clean up the mess.
		 */
        *pprev = tmp;
        pprev = &tmp->vm_next;

        if (retval)
            goto fail_nomem;
    }
    retval = 0;
#if 0
    if (mm->map_count >= AVL_MIN_MAP_COUNT)
        build_mmap_avl(mm);
#endif

fail_nomem:
    //flush_tlb_mm(current->mm);
    return retval;
}

static int copy_mm(unsigned long clone_flags, struct task_struct *tsk)
{
    struct mm_struct *mm, *oldmm;
    int retval;

    tsk->min_flt = tsk->maj_flt = 0;
    tsk->cmin_flt = tsk->cmaj_flt = 0;
    tsk->nswap = tsk->cnswap = 0;

    tsk->mm = NULL;
    tsk->active_mm = NULL;

    /*
	 * Are we cloning a kernel thread?
	 *
	 * We need to steal a active VM for that..
	 */
    oldmm = current->mm;
    if (!oldmm)
        return 0;
#if VFORK
    if (clone_flags & CLONE_VM)
    {
        atomic_inc(&oldmm->mm_users);
        mm = oldmm;
        goto good_mm;
    }
#endif
    retval = -ENOMEM;
    mm = allocate_mm();
    if (!mm)
        goto fail_nomem;

    /* Copy the current MM stuff.. */
    memcpy(mm, oldmm, sizeof(*mm));
    if (!mm_init(mm))
        goto fail_nomem;

    //down(&oldmm->mmap_sem);
    retval = dup_mmap(mm);
    //up(&oldmm->mmap_sem);

    /*
	 * Add it to the mmlist after the parent.
	 *
	 * Doing it this way means that we can order
	 * the list, and fork() won't mess up the
	 * ordering significantly.
	 */
    spin_lock(&mmlist_lock);
    list_add(&mm->mmlist, &oldmm->mmlist);
    spin_unlock(&mmlist_lock);

    if (retval)
        goto free_pt;

    /*
	 * child gets a private LDT (if there was an LDT in the parent)
	 */
    //copy_segments(tsk, mm);

//    if (init_new_context(tsk, mm))
//        goto free_pt;

//good_mm:
    tsk->mm = mm;
    tsk->active_mm = mm;
    show_mm(mm);
    return 0;

free_pt:
    mmput(mm);
fail_nomem:

    return retval;
}

int get_pid(unsigned long flags)
{
    static unsigned int pid = 1;

    return pid++;
}

void copy_flags(unsigned long clone_flags, struct task_struct *p)
{
    unsigned long new_flags = p->flags;

    new_flags &= ~(PF_SUPERPRIV | PF_USEDFPU | PF_VFORK);
    new_flags |= PF_FORKNOEXEC;

#if VFORK
    if (!(clone_flags & CLONE_PTRACE))
        p->ptrace = 0;
    if (clone_flags & CLONE_VFORK)
        new_flags |= PF_VFORK;
#endif
    p->flags = new_flags;
}

/*
 *  Ok, this is the main fork-routine. It copies the system process
 * information (task[nr]) and sets up the necessary registers. It also
 * copies the data segment in its entirety.  The "stack_start" and
 * "stack_top" arguments are simply passed along to the platform
 * specific copy_thread() routine.  Most platforms ignore stack_top.
 * For an example that's using stack_top, see
 * arch/ia64/kernel/process.c.
 */
int do_fork(unsigned long clone_flags, unsigned long stack_start, struct pt_regs *regs, unsigned long stack_size)
{
    int retval = -ENOMEM;
    struct task_struct *p;

#if VFORK
    DECLARE_MUTEX_LOCKED(sem);

    if (clone_flags & CLONE_PID)
    {
        /* This is only allowed from the boot up thread */
        if (current->pid)
            return -EPERM;
    }

    current->vfork_sem = &sem;
#endif

    p = alloc_task_struct();
    if (!p)
        goto fork_out;

    *p = *current;

    retval = -EAGAIN;
#if 0
    if (atomic_read(&p->user->processes) >= p->rlim[RLIMIT_NPROC].rlim_cur)
        goto bad_fork_free;
    atomic_inc(&p->user->__count);
    atomic_inc(&p->user->processes);

    /*
	 * Counter increases are protected by
	 * the kernel lock so nr_threads can't
	 * increase under us (but it may decrease).
	 */
    if (nr_threads >= max_threads)
        goto bad_fork_cleanup_count;

    get_exec_domain(p->exec_domain);

    if (p->binfmt && p->binfmt->module)
        __MOD_INC_USE_COUNT(p->binfmt->module);
#endif

    p->did_exec = 0;
    //p->swappable = 0;
    p->state = TASK_UNINTERRUPTIBLE;

    copy_flags(clone_flags, p);
    p->pid = get_pid(clone_flags);

    p->run_list.next = NULL;
    p->run_list.prev = NULL;

#if VFORK
    if ((clone_flags & CLONE_VFORK) || !(clone_flags & CLONE_PARENT))
    {
        p->p_opptr = current;
        if (!(p->ptrace & PT_PTRACED))
            p->p_pptr = current;
    }
    
    p->vfork_sem = NULL;
#endif

    p->p_cptr = NULL;
    init_waitqueue_head(&p->wait_childexit);
    spin_lock_init(&p->alloc_lock);

#if SIGNAL
    p->sigpending = 0;
    init_sigpending(&p->pending);
#endif

#if TIMER
    p->it_real_value = p->it_virt_value = p->it_prof_value = 0;
    p->it_real_incr = p->it_virt_incr = p->it_prof_incr = 0;
    init_timer(&p->real_timer);
    p->real_timer.data = (unsigned long)p;
#endif

#if 0
    p->leader = 0; /* session leadership doesn't inherit */
    p->tty_old_pgrp = 0;
    p->times.tms_utime = p->times.tms_stime = 0;
    p->times.tms_cutime = p->times.tms_cstime = 0;
#endif

#ifdef CONFIG_SMP
    {
        int i;
        p->has_cpu = 0;
        p->processor = current->processor;
        /* ?? should we just memset this ?? */
        for (i = 0; i < smp_num_cpus; i++)
            p->per_cpu_utime[i] = p->per_cpu_stime[i] = 0;
        spin_lock_init(&p->sigmask_lock);
    }
#endif

    p->lock_depth = -1; /* -1 = no lock */
    p->start_time = jiffies;

    retval = -ENOMEM;

#if FILE
    /* copy all the process information */
    if (copy_files(clone_flags, p))
        goto bad_fork_cleanup;
    if (copy_fs(clone_flags, p))
        goto bad_fork_cleanup_files;
#endif
#if SIGNAL
    if (copy_sighand(clone_flags, p))
        goto bad_fork_cleanup_fs;
#endif
    if (copy_mm(clone_flags, p))
        goto bad_fork_cleanup_sighand;
    retval = copy_thread(0, clone_flags, stack_start, stack_size, p, regs);
    if (retval)
        goto bad_fork_cleanup_sighand;
    //p->semundo = NULL;

    /* Our parent execution domain becomes current domain
	   These must match for thread signalling to apply */

    //p->parent_exec_id = p->self_exec_id;

#if 0
    /* ok, now we should be set up.. */
    p->swappable = 1;
    p->exit_signal = clone_flags & CSIGNAL;
    p->pdeath_signal = 0;
#endif

    /*
	 * "share" dynamic priority between parent and child, thus the
	 * total amount of dynamic priorities in the system doesnt change,
	 * more scheduling fairness. This is only important in the first
	 * timeslice, on the long run the scheduling behaviour is unchanged.
	 */
    p->counter = (current->counter + 1) >> 1;
    current->counter >>= 1;
    if (!current->counter)
        current->need_resched = 1;

    /*
	 * Ok, add it to the run-queues and make it
	 * visible to the rest of the system.
	 *
	 * Let it rip!
	 */
    retval = p->pid;
#if 0
    p->tgid = retval;
    INIT_LIST_HEAD(&p->thread_group);
    write_lock_irq(&tasklist_lock);
    if (clone_flags & CLONE_THREAD)
    {
        p->tgid = current->tgid;
        list_add(&p->thread_group, &current->thread_group);
    }
    SET_LINKS(p);
    hash_pid(p);
#endif
    nr_threads++;

#if 0
    write_unlock_irq(&tasklist_lock);

    if (p->ptrace & PT_PTRACED)
        send_sig(SIGSTOP, p, 1);
#endif
    wake_up_process(p); /* do this last */
    ++total_forks;

fork_out:
#if VFORK
    if ((clone_flags & CLONE_VFORK) && (retval > 0))
        down(&sem);
#endif
    return retval;

bad_fork_cleanup_sighand:
#if SIGNAL
    exit_sighand(p);
#endif

#if FILE
bad_fork_cleanup_fs:
    exit_fs(p); /* blocking */
bad_fork_cleanup_files:
    exit_files(p); /* blocking */
#endif

#if 0
bad_fork_cleanup:
    put_exec_domain(p->exec_domain);
    if (p->binfmt && p->binfmt->module)
        __MOD_DEC_USE_COUNT(p->binfmt->module);
bad_fork_cleanup_count:
    atomic_dec(&p->user->processes);
    free_uid(p->user);
#endif

//bad_fork_free:
    free_task_struct(p);
    goto fork_out;
}