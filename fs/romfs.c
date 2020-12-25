#include "lib.h"
#include "printk.h"
#include "blk.h"
#include "fs.h"
#include "error.h"

void romfs_read_inode(struct inode *i);
int  romfs_open(struct inode *inode, struct file *fp);
void romfs_close(struct inode *inode, struct file *fp);
int  romfs_read(struct inode *inode, struct file *fp, char *buf, int len);
int romfs_lookup(struct inode *dir, char *name, int name_len, struct inode **result);

#define ROMFH_SIZE 16
#define ROMFH_PAD (ROMFH_SIZE - 1)
#define ROMFH_MASK (~ROMFH_PAD)

#define ROMFH_TYPE 7
#define ROMFH_HRD  0 /* 硬链接文件 */
#define ROMFH_DIR  1 /* 目录文件 */
#define ROMFH_REG  2 /* 普通文件 */
#define ROMFH_LNK  3 /* 链接文件 */
#define ROMFH_BLK  4 /* 块设备文件 */
#define ROMFH_CHR  5 /* 字符设备文件 */
#define ROMFH_SCK  6 /* 套接字文件 */
#define ROMFH_FIF  7 /* 管道文件 */

struct romfs_super_block
{
    char magic[8];
    unsigned int size;
    unsigned int checksum;
    char name[0];
};

struct romfs_inode
{
    unsigned int next;
    unsigned int spec;
    unsigned int size;
    unsigned int checksum;
    char name[16];
};


// TODO FIXME 
char static_rw_buf[1024 *1024];
static int romfs_copyfrom(struct inode *i, void *dest, unsigned long offset, unsigned long count)
{
    int ret;
    unsigned long bc;
    unsigned long off;

    bc = count / SYSTEM_DEFAULT_SECTOR_SIZE;
    if (count % SYSTEM_DEFAULT_SECTOR_SIZE)
        bc++;
    
    if ((bc * SYSTEM_DEFAULT_SECTOR_SIZE - (offset % SYSTEM_DEFAULT_SECTOR_SIZE)) < count)
        bc++;

    off = offset / SYSTEM_DEFAULT_SECTOR_SIZE;

    ret = block_read(i, off, static_rw_buf, bc);
    if (ret > 0)
        memcpy(dest, static_rw_buf + offset % SYSTEM_DEFAULT_SECTOR_SIZE, (count <= ret) ? count : ret);

    return (count <= ret) ? count : ret;
}

struct file_operations romfs_file_operations = 
{
    romfs_open,
    romfs_close,
    romfs_read,
    NULL,
    NULL,
    NULL,
    NULL,
};
struct inode_operations romfs_inode_operations = 
{
    &romfs_file_operations, 
    NULL,
    romfs_lookup,
    NULL,
    NULL,
    NULL,
    NULL,
};

struct super_operations romfs_super_s_op = 
{
    romfs_read_inode,
};


void romfs_read_inode(struct inode *i)
{
    int nextfh, ino;
    struct romfs_inode ri;

    ino = i->i_ino & ROMFH_MASK;
    i->i_mode = 0;

    /* Loop for finding the real hard link */
    for (;;)
    {
        if (romfs_copyfrom(i, &ri, ino, ROMFH_SIZE) <= 0)
        {
            printk("romfs: read error for inode 0x%x\n", ino);
            return;
        }
        /* XXX: do romfs_checksum here too (with name) */

        nextfh = ntohl(ri.next);
        if ((nextfh & ROMFH_TYPE) != ROMFH_HRD)
            break;

        ino = ntohl(ri.spec) & ROMFH_MASK;
    }

    //i->i_nlink = 1; /* Hard to decide.. */
    i->i_size = ntohl(ri.size);
    //i->i_mtime = i->i_atime = i->i_ctime = 0;
    
    //i->i_uid = i->i_gid = 0;

    //i->u.romfs_i.i_metasize = ino;
    //i->u.romfs_i.i_dataoffset = ino + (i->i_ino & ROMFH_MASK);

    /* Compute permissions */
    //ino = romfs_modemap[nextfh & ROMFH_TYPE];
    /* only "normal" files have ops */
    switch (nextfh & ROMFH_TYPE)
    {
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    default:
        break;
    }
}

int romfs_lookup(struct inode *dir, char *name, int name_len, struct inode **result)
{
    struct inode *inode;
    struct romfs_inode *romfs_inode;
    char *buf;
    int ret;
    unsigned int next;
   
    buf = kmalloc(sizeof (struct romfs_inode), GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    next = dir->i_ino + 0x20;
    do
    {
        ret = romfs_copyfrom(dir, buf, next, sizeof (struct romfs_inode));
        if (ret < 0)
        {
            kfree(buf);
            return ret;
        }
        romfs_inode = (struct romfs_inode *)(buf);
        
        printk("file name: %s\n", romfs_inode->name);
        printk("file spec: %d(0x%x)\n", ntohl(romfs_inode->spec), ntohl(romfs_inode->spec));
        printk("file size: %d(0x%x)\n", ntohl(romfs_inode->size), ntohl(romfs_inode->size));
        printk("file type: %d(0x%x)\n", next & 0xf, next & 0xf);


        if (strncmp(romfs_inode->name, name, name_len) == 0)
            break;

        next = ntohl(romfs_inode->next);
        
        printk("file next: %d(0x%x)\n", next, next);
        switch (next & 0x7)
        {
        case ROMFH_HRD:
            printk("Hard Link");
            break;
        case ROMFH_DIR:
            printk("Directory");
            break;
        case ROMFH_REG:
            printk("Common");
            break;
        default:
            break;
        }
        printk("\n");
        next = next & ~0xf;
    } while (next != 0x0a);

    if (next == 0x0a)
    {
        kfree(buf);
        return -ENOENT;
    }

    inode = iget(dir->i_sb, next, NULL, NULL);
    if (!inode)
    {
        kfree(buf);
        return -ENOMEM;
    }

    inode->i_size = ntohl(romfs_inode->size);
    inode->i_what = buf;
    inode->i_ino = next;
    inode->i_op = &romfs_inode_operations;
    *result = inode;

    return 0;
}

int  romfs_open(struct inode *inode, struct file *fp)
{
    return 0;
}

void romfs_close(struct inode *inode, struct file *fp)
{

}

int  romfs_read(struct inode *inode, struct file *fp, char *buf, int len)
{
    int ret;
    unsigned long read_size = len;

    if (fp->f_pos >= inode->i_size)
        return 0;
    
    if (fp->f_pos + len > inode->i_size)
        read_size = inode->i_size - fp->f_pos;

    ret = romfs_copyfrom(inode, buf, inode->i_ino + 0x20 + fp->f_pos, read_size);
    if (ret < 0)
        return ret;

    fp->f_pos += read_size;
    return read_size;
}

struct super_block *romfs_read_super(struct super_block *sb)
{
    int ret;
    char *buff;
    int disk_secotr_size;
    struct inode *inode;
    struct romfs_inode *romfs_inode;
    struct romfs_super_block *romfs_sb;

    inode = kmalloc(sizeof (struct inode), GFP_KERNEL);
    if (!inode)
        return NULL;
    
    romfs_inode = kmalloc(sizeof (struct romfs_inode), GFP_KERNEL);
    if (!romfs_inode)
        goto err_kmalloc_romfs_inode;
    
    romfs_sb = kmalloc(sizeof (struct romfs_super_block), GFP_KERNEL);
    if (!romfs_sb)
        goto err_kmalloc_romfs_sb;
   
    inode->i_dev = sb->s_dev;

//    disk_secotr_size = get_disk_sector_size();

    disk_secotr_size = 512;
    buff = kmalloc(disk_secotr_size, GFP_KERNEL);
    if (!buff)
        goto err_kmalloc_buffer;

    ret = romfs_copyfrom(inode, buff, 0, sizeof (struct romfs_super_block));
    if (ret < 0)
        goto err_block_read;
    
    memcpy(romfs_sb, buff, sizeof (struct romfs_super_block));

    inode->i_op = &romfs_inode_operations;
    inode->i_what = romfs_inode;
    inode->i_sb = sb;
    inode->i_ino = 0x20;
    
    sb->s_type = FILE_SYSTEM_TYPE_ROMFS;
    sb->s_mounted = inode;
    sb->sb_data = romfs_sb;
    kfree(buff);
    
    return sb;

err_block_read:
    kfree(buff);
err_kmalloc_buffer:
    kfree(romfs_sb);
err_kmalloc_romfs_sb:
    kfree(romfs_inode);
err_kmalloc_romfs_inode:
    kfree(inode);

    return NULL;
}

