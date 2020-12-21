#ifndef __BINFMTS_H__
#define __BINFMTS_H__

#include "fs.h"
#include "page.h"
#include "ptrace.h"

#define MAX_ARG_PAGES 32
#define BINPRM_BUF_SIZE 128


struct linux_binprm
{
    char buf[BINPRM_BUF_SIZE];
    struct page *page[MAX_ARG_PAGES];
    unsigned long p; /* current top of mem */
    int sh_bang;
    struct file *file;
    int e_uid, e_gid;
    //kernel_cap_t cap_inheritable, cap_permitted, cap_effective;
    int argc, envc;
    char *filename; /* Name of binary */
    unsigned long loader, exec;
};

struct linux_binfmt
{
    struct linux_binfmt *next;
    //struct module *module;
    int (*load_binary)(struct linux_binprm *, struct pt_regs *regs);
    int (*load_shlib)(struct file *);
    int (*core_dump)(long signr, struct pt_regs *regs, struct file *file);
    unsigned long min_coredump; /* minimal dump size */
};

#endif
