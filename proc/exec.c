#include <string.h>

#include "fs.h"
#include "error.h"
#include "sched.h"
#include "binfmts.h"
#include "common.h"

static struct linux_binfmt *formats;

#if 1
int exec_mmap(void)
{
    struct mm_struct *old_mm;

    old_mm = current->mm;
    if (old_mm && atomic_read(&old_mm->mm_users) == 1)
    {
        //flush_cache_mm(old_mm);//TODO fix me here
        //mm_release();
        exit_mmap(old_mm);
        //flush_tlb_mm(old_mm);//TODO fix me here
        return 0;
    }
#if 0
	mm = mm_alloc();
	if (mm) {
		struct mm_struct *active_mm = current->active_mm;

		if (init_new_context(current, mm)) {
			mmdrop(mm);
			return -ENOMEM;
		}

		/* Add it to the list of mm's */
		spin_lock(&mmlist_lock);
		list_add(&mm->mmlist, &init_mm.mmlist);
		spin_unlock(&mmlist_lock);

		task_lock(current);
		current->mm = mm;
		current->active_mm = mm;
		task_unlock(current);
		activate_mm(active_mm, mm);
		mm_release();
		if (old_mm) {
			if (active_mm != old_mm) BUG();
			mmput(old_mm);
			return 0;
		}
		mmdrop(active_mm);
		return 0;
	}
	return -ENOMEM;
#endif

    return -ENOMEM;
}

int search_binary_handler(struct linux_binprm *bprm, struct pt_regs *regs)
{
    int ret;
    struct linux_binfmt *fmt;

    for (fmt = formats; fmt; fmt = fmt->next)
    {
        int (*fn)(struct linux_binprm *, struct pt_regs *) = fmt->load_binary;
        if (!fn)
            continue;

        ret = fn(bprm, regs);
        if (ret > 0)
            return ret;
    }

    return -ENOEXEC;
}

int kernel_read(struct file *file, unsigned long offset, char *addr, unsigned long count)
{
    //unsigned int pos = offset;

    if (!file || !file->f_op || !file->f_op->read)
        return -ENOSYS;
    return file->f_op->read(file->f_inode, file, addr, count);
}

int prepare_binprm(struct linux_binprm *bprm)
{
    int mode;
    struct inode *inode = bprm->file->f_inode;

    mode = inode->i_mode;
    /* Huh? We had already checked for MAY_EXEC, WTF do we check this? */
    if (!(mode & 0111)) /* with at least _one_ execute bit set */
        return -EACCES;
    if (bprm->file->f_op == NULL)
        return -EACCES;

    memset(bprm->buf, 0, BINPRM_BUF_SIZE);
    return kernel_read(bprm->file, 0, bprm->buf, BINPRM_BUF_SIZE);
}

struct file *open_exec(char *name)
{
    struct file *file = NULL;

    return file;
}

int do_execve(char *filename, char **argv, char **envp, struct pt_regs *regs)
{
    struct linux_binprm bprm;
    struct file *file;
    int retval;
    int i;

    file = open_exec(filename);
    if (!file)
        return -ENFILE;

    bprm.p = PAGE_SIZE * MAX_ARG_PAGES - sizeof(void *);
    memset(bprm.page, 0, MAX_ARG_PAGES * sizeof(bprm.page[0]));

    bprm.file = file;
    bprm.filename = filename;
    bprm.sh_bang = 0;
    bprm.loader = 0;
    bprm.exec = 0;

#if 0
    if ((bprm.argc = count(argv, bprm.p / sizeof(void *))) < 0)
    {
        allow_write_access(file);
        fput(file);
        return bprm.argc;
    }

    if ((bprm.envc = count(envp, bprm.p / sizeof(void *))) < 0)
    {
        allow_write_access(file);
        fput(file);
        return bprm.envc;
    }
#endif
    retval = prepare_binprm(&bprm);
    if (retval < 0)
        goto out;
#if 0
    retval = copy_strings_kernel(1, &bprm.filename, &bprm);
    if (retval < 0)
        goto out;

    bprm.exec = bprm.p;
    retval = copy_strings(bprm.envc, envp, &bprm);
    if (retval < 0)
        goto out;

    retval = copy_strings(bprm.argc, argv, &bprm);
    if (retval < 0)
        goto out;
#endif
    retval = search_binary_handler(&bprm, regs);
    if (retval >= 0)
        /* execve success */
        return retval;

out:

#if 0
    /* Something went wrong, return the inode and free the argument pages*/
    allow_write_access(bprm.file);
    if (bprm.file)
        fput(bprm.file);
#endif

    for (i = 0; i < MAX_ARG_PAGES; i++)
    {
        struct page *page = bprm.page[i];
        if (page)
            __free_page(page);
    }

    return retval;
}
#endif