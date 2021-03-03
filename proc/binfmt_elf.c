#include "elf.h"
#include "fs.h"
#include "error.h"
#include "sched.h"
#include "binfmts.h"
#include "common.h"
#include "kmalloc.h"


int load_elf_binary(struct linux_binprm * bprm, struct pt_regs * regs)
{
    int error;
    unsigned long  text_addr, map_size, i;
    Elf32_Ehdr *ehdr;
    Elf32_Phdr *phdr;
    char *ph_buf;
    unsigned long ph_size;
    

    ehdr = (Elf32_Ehdr *)bprm->buf;
    ph_size = ehdr->e_phnum * ehdr->e_phentsize;
    ph_buf = kmalloc(ph_size, 0);

    //TODO set file offset
    bprm->file->f_pos = ehdr->e_phoff;
    error = bprm->file->f_op->read(bprm->file->f_inode, bprm->file, ph_buf, ph_size); 
    if (error < 0)
    {
        //send_sig(SIGKILL, current, 0);
        return error;
    }

    for (i = 0; i < ehdr->e_phnum; i++)
    {
        phdr = (Elf32_Phdr *)(ph_buf + i * ehdr->e_phentsize);
        
        if (phdr->p_type == PT_LOAD)
        {
            text_addr = phdr->p_vaddr;
            map_size = phdr->p_memsz;

            error = do_brk(text_addr & PAGE_MASK, map_size);
            if (error != (text_addr & PAGE_MASK))
            {
                //send_sig(SIGKILL, current, 0);
                return error;
            }

            //TODO set file offset

            bprm->file->f_pos = phdr->p_offset;
            error = bprm->file->f_op->read(bprm->file->f_inode, bprm->file, (char *)text_addr, phdr->p_filesz);
            if (error < 0)
            {
                //send_sig(SIGKILL, current, 0);
                return error;
            }
        }

    }

    return 0;
}


static struct linux_binfmt elf_format = 
{
    .load_binary = load_elf_binary,
};

extern void linux_binfmt_register(struct linux_binfmt *fmt);
void elf_fmt_init(void)
{
    linux_binfmt_register(&elf_format);
}