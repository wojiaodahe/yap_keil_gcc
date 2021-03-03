#include "aout.h"
#include "fs.h"
#include "error.h"
#include "sched.h"
#include "binfmts.h"
#include "common.h"

struct exec
{
  unsigned long a_info;		/* Use macros N_MAGIC, etc for access */
  unsigned a_text;		/* length of text, in bytes */
  unsigned a_data;		/* length of data, in bytes */
  unsigned a_bss;		/* length of uninitialized data area for file, in bytes */
  unsigned a_syms;		/* length of symbol table data in file, in bytes */
  unsigned a_entry;		/* start address */
  unsigned a_trsize;		/* length of relocation info for text, in bytes */
  unsigned a_drsize;		/* length of relocation info for data, in bytes */
};

#define N_TRSIZE(a)	((a).a_trsize)
#define N_DRSIZE(a)	((a).a_drsize)
#define N_SYMSIZE(a)	((a).a_syms)

#define STACK_TOP	TASK_SIZE

int load_aout_binary(struct linux_binprm * bprm, struct pt_regs * regs)
{
    int error;
    unsigned long  text_addr, map_size;
	struct exec ex;


    return 0;

    ex = *((struct exec *) bprm->buf);
    map_size = ex.a_text + ex.a_data;

    text_addr = N_TXTADDR(ex);

    error = do_brk(text_addr & PAGE_MASK, map_size);
    if (error != (text_addr & PAGE_MASK))
    {
        //send_sig(SIGKILL, current, 0);
        return error;
    }

    //TODO set file offset

    *(volatile unsigned int * )0 = 10000;
    error = bprm->file->f_op->read(bprm->file->f_inode, bprm->file, (char *)text_addr, ex.a_text + ex.a_data);
    if (error < 0)
    {
        //send_sig(SIGKILL, current, 0);
        return error;
    }

    return 0;
}


static struct linux_binfmt aout_format = 
{
    .load_binary = load_aout_binary,
};

extern void linux_binfmt_register(struct linux_binfmt *fmt);
void aout_fmt_init(void)
{
    linux_binfmt_register(&aout_format);
}