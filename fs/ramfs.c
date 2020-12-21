#include "fs.h"
#include "error.h"
#include "ramfs.h"
#include "blk.h"
#include "kmalloc.h"
#include "common.h"
#include "lib.h"


int  ramfs_open   (struct inode *, struct file *);
void ramfs_close (struct inode *, struct file *);
int ramfs_read(struct inode *inode, struct file *filp, char *buf, int len);
int ramfs_write(struct inode *inode, struct file *filp, char *buf, int len);
int ramfs_lookup(struct inode *dir, char *name, int namelen, struct inode **res_inode);
int ramfs_create(struct inode *dir, char *name, int namelen, int mode, struct inode **res_inode);
int ramfs_mknod(struct inode *dir, char *name, int namelen, int mode, int dev_num);
int ramfs_mkdir(struct inode *dir, char *name, int namelen, int mode);

struct file_operations ramfs_file_operations = 
{
    ramfs_open,
    ramfs_close,
    ramfs_read,
    ramfs_write,
    NULL,
    NULL,
    NULL,
};

struct inode_operations ramfs_inode_operations = 
{
    &ramfs_file_operations, 
    ramfs_create,
    ramfs_lookup,
    ramfs_mkdir,
    NULL,
    ramfs_mknod,
    NULL,
};


struct ramfs_inode *ramfs_init()
{
	return NULL;
}

int ramfs_open(struct inode *inode, struct file *filp)
{
    int ret = 0;
    return ret;
}

void ramfs_close(struct inode *inode, struct file *filp)
{

}

#if 0
int ramfs_read(struct inode *inode, struct file *filp, char *buf, int len)
{
    int ret;

    ret = block_read(inode, filp->f_pos, buf, len);

    if (ret < 0)
        return ret;

    filp->f_pos += ret;

    return ret;
}
#else
int ramfs_read(struct inode *inode, struct file *filp, char *buf, int len)
{
    int ret;
    char *buff;
    int block_size;
    int rw_block;
    int nr_blocks = 0;
    struct ramfs_inode *r_inode;

    if (!len)
        return len;
    
    if (!inode)
        return -EINVAL;
	
    r_inode = inode->i_what;
    block_size = SYSTEM_DEFAULT_SECTOR_SIZE;
    
    rw_block = filp->f_pos / block_size;

    if ((filp->f_pos + len) > r_inode->size)
        len = r_inode->size - filp->f_pos;

    nr_blocks += len / block_size;
    if (len % block_size)
        nr_blocks++;
#if 0 
	if (filp->f_pos % block_size)
        nr_blocks++;
#else
	if ((nr_blocks * block_size - (filp->f_pos % block_size)) < len)
		nr_blocks++;
#endif

    buff = kmalloc(nr_blocks * block_size);
#if 0
    printk("\nfilp->f_pos: %d\n", filp->f_pos);
    printk("len: %d\n", len);
    printk("nr_blocks: %d\n", nr_blocks);
    printk("rw_block: %d\n", rw_block);
    printk("filp offset: %d\n", filp->f_pos % block_size);
#endif
    ret = block_read(inode, rw_block, buff, nr_blocks);
    if (ret < 0)
    {   
        kfree(buff);
        return ret;
    }

    memcpy(buf, buff + filp->f_pos % block_size, len);
    kfree(buff);

    filp->f_pos += len;

    return len;
}
#endif

int ramfs_write(struct inode *inode, struct file *filp, char *buf, int len)
{
    int ret;
    char *buff;
    int block_size;
    int rw_block;
    int nr_blocks = 0;
    struct ramfs_inode *r_inode;

    if (!len)
        return len;
    
    if (!inode)
        return -EINVAL;
	
    r_inode = inode->i_what;
    block_size = SYSTEM_DEFAULT_SECTOR_SIZE;
    
    rw_block = filp->f_pos / block_size;

    if ((filp->f_pos + len) > r_inode->size)
        len = r_inode->size - filp->f_pos;

    nr_blocks += len / block_size;
    if (len % block_size)
        nr_blocks++;
#if 0 
	if (filp->f_pos % block_size)
        nr_blocks++;
#else
	if ((nr_blocks * block_size - (filp->f_pos % block_size)) < len)
		nr_blocks++;
#endif

    buff = kmalloc(nr_blocks * block_size);
#if 0
    printk("\nfilp->f_pos: %d\n", filp->f_pos);
    printk("len: %d\n", len);
    printk("nr_blocks: %d\n", nr_blocks);
    printk("rw_block: %d\n", rw_block);
    printk("filp offset: %d\n", filp->f_pos % block_size);
#endif
    ret = block_read(inode, rw_block, buff, 1);
    if (ret < 0)
    {   
        kfree(buff);
        return ret;
    }

    memcpy(buff +  filp->f_pos % block_size, buf, len);
    ret = block_write(inode, rw_block, buff, nr_blocks);
    if (ret < 0)
    {   
        kfree(buff);
        return ret;
    }

    kfree(buff);

    filp->f_pos += len;

    return len;
}

struct ramfs_inode *ramfs_travel_child(struct ramfs_inode *parent,   char *name, int namelen)
{
    struct ramfs_inode *child;
    if (!parent)
        return NULL;
    
    child = parent->child;

    while (child != NULL)
    {
        if (strncmp(child->name, name, namelen) == 0)
            return child;
        child = child->next;
    }

    return NULL;
}

int ramfs_lookup(struct inode *dir,   char *name, int namelen, struct inode **res_inode)
{
    struct ramfs_inode *parent;
    struct ramfs_inode *child;

    parent = dir->i_what;

    child = ramfs_travel_child(parent, name, namelen);

    if (!child)
        return -1;

    *res_inode = child->inode;
    
    return 0;
}

int ramfs_add_node(struct ramfs_inode *parent, struct ramfs_inode *child)
{
   struct ramfs_inode *tmp; 

   tmp = parent->child;

   if (tmp == NULL)
   {
       parent->child = child;
       return 0;
   }
   while (tmp->next != NULL)
       tmp = tmp->next;

   tmp->next = child;
   child->prev = tmp;
   child->next = NULL;

   return 0;
}

int ramfs_create(struct inode *dir, char *name, int namelen, int mode, struct inode **res_inode)
{
    struct ramfs_inode *parent;
    struct ramfs_inode *child;
    struct inode       *inode;

    parent = dir->i_what;

    if (!parent)
        return -1;

    child = ramfs_travel_child(parent, name, namelen);
    if (child)
        return -1;

    child = kmalloc(sizeof (struct ramfs_inode));
    if (!child)
        return -1;
    memset(child, 0, sizeof (*child));

    inode = kmalloc(sizeof (struct inode));
    if (!inode)
        return -1;
    memset(inode, 0, sizeof (*inode));

    memcpy(child->name, name, namelen);
   
    child->inode = inode;
    inode->i_what = child;
	inode->i_dev = dir->i_dev;

    inode->i_op = &ramfs_inode_operations;

	*res_inode = inode;

    child->start_addr = kmalloc(RAMFS_DEFAULT_FILE_LEN);
    memset(child->start_addr, 0, RAMFS_DEFAULT_FILE_LEN);
    child->size = RAMFS_DEFAULT_FILE_LEN;

    return ramfs_add_node(parent, child);
}
extern struct inode_operations chrdev_inode_operations;
extern struct inode_operations blkdev_inode_operations;

int ramfs_mkdir(struct inode *dir,   char *name, int namelen, int mode)
{
    struct ramfs_inode *parent;
    struct ramfs_inode *child;
    struct inode       *inode;

    parent = dir->i_what;

    if (!parent)
        return -1;

    child = ramfs_travel_child(parent, name, namelen);
    if (child)
        return -1;

    child = kmalloc(sizeof (struct ramfs_inode));
    if (!child)
        return -1;
    memset(child, 0, sizeof(*child));

    inode = kmalloc(sizeof (struct inode));
    if (!inode)
        return -1;
    memset(inode, 0, sizeof (*inode));

    memcpy(child->name, name, namelen);
   
    child->inode = inode;
    inode->i_what = child;
	inode->i_dev = dir->i_dev;

    inode->i_op = &ramfs_inode_operations;

    mode &= ~(S_IFMT);
    mode |= S_IFDIR;

    inode->i_mode = mode;

    child->start_addr = kmalloc(RAMFS_DEFAULT_FILE_LEN);
    child->size = RAMFS_DEFAULT_FILE_LEN;

    return ramfs_add_node(parent, child);
}

int ramfs_mknod(struct inode *dir,   char *name, int namelen, int mode, int dev_num)
{
    struct ramfs_inode *parent;
    struct ramfs_inode *child;
    struct inode       *inode;
    
    if (!dir)
        return -ENOENT;

    parent = dir->i_what;

    if (!parent)
        return -ENOENT;

    child = ramfs_travel_child(parent, name, namelen);
    if (child)
        return -EBUSY;

    child = kmalloc(sizeof (struct ramfs_inode));
    if (!child)
        return -ENOSPC;
    memset(child, 0, sizeof(*child));
    memcpy(child->name, name, namelen);

    inode = kmalloc(sizeof (struct inode));
    if (!inode)
    {
        kfree(child);
        return -ENOSPC;
    }
    memset(inode, 0, sizeof (*inode));
	inode->i_mode = mode;
	inode->i_op = NULL;
	inode->i_dev = dev_num;
    inode->i_what = child;
    child->inode = inode;
   
    if (S_ISCHR(inode->i_mode)) 
        inode->i_op = &chrdev_inode_operations;
    else if (S_ISBLK(inode->i_mode))
        inode->i_op = &blkdev_inode_operations;
    
    return ramfs_add_node(parent, child);
}

struct super_block *ramfs_read_super(struct super_block *sb)
{
    struct inode *inode;
    struct ramfs_inode *ramfs_inode;

    inode = kmalloc(sizeof (struct inode));
    if (!inode)
        return NULL;
    memset(inode, 0, sizeof (*inode));
    
    ramfs_inode = kmalloc(sizeof (struct ramfs_inode));
    if (!ramfs_inode)
    {
        kfree(inode);
        return NULL;
    }
    memset(ramfs_inode, 0, sizeof (*ramfs_inode));

    inode->i_dev = sb->s_dev;

    inode->i_op = &ramfs_inode_operations;
	inode->i_what = ramfs_inode;
    ramfs_inode->inode = inode;
    
    sb->s_mounted = inode;
    sb->s_type = FILE_SYSTEM_TYPE_RAMFS;

    return sb;
}

