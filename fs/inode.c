#include "string.h"
#include "malloc.h"

#include "fs.h"
#include "list.h"
#include "common.h"
#include "atomic.h"


static void destroy_inode(struct inode *inode)
{
    //kmem_cache_free(inode_cachep, (inode));
}

static struct inode *alloc_inode()
{
    return (struct inode *)malloc(sizeof (struct inode));
}

static void __iget(struct inode * inode)
{
    if (atomic_read(&inode->i_count))
    {
        atomic_inc(&inode->i_count);
        return;
    }
    atomic_inc(&inode->i_count);

#if 0
    if (!(inode->i_state & I_DIRTY))
    {
        list_del(&inode->i_list);
        list_add(&inode->i_list, &inode_in_use);
    }
    inodes_stat.nr_unused--;
#endif
}

static void clean_inode(struct inode *inode)
{
#if 0
	static struct address_space_operations empty_aops;
	static struct inode_operations empty_iops;
	static struct file_operations empty_fops;

	memset(&inode->u, 0, sizeof(inode->u));
	inode->i_sock = 0;
	inode->i_op = &empty_iops;
	inode->i_fop = &empty_fops;
	inode->i_nlink = 1;
	atomic_set(&inode->i_writecount, 0);
	inode->i_size = 0;
	inode->i_generation = 0;
	memset(&inode->i_dquot, 0, sizeof(inode->i_dquot));
	inode->i_pipe = NULL;
	inode->i_bdev = NULL;
	inode->i_data.a_ops = &empty_aops;
	inode->i_data.host = inode;
	inode->i_mapping = &inode->i_data;
#endif

    memset(inode, 0, sizeof (*inode));

    INIT_LIST_HEAD(&inode->i_list);
}

static struct inode *find_inode(struct super_block *sb, unsigned long ino, struct list_head *head, find_inode_t find_actor, void *arg)
{

    struct list_head *tmp;
    struct inode *inode;

    tmp = head;
    for (;;)
    {
        tmp = tmp->next;
        inode = NULL;
        if (tmp == head)
            break;
        inode = list_entry(tmp, struct inode, i_list);
        if (inode->i_ino != ino)
            continue;
        if (inode->i_sb != sb)
            continue;
        if (find_actor && !find_actor(inode, ino, arg))
            continue;
        break;
    }
    return inode;
}

static struct inode *get_new_inode(struct super_block *sb, unsigned long ino, struct list_head *head, find_inode_t find_actor, void *opaque)
{
    struct inode *inode;

    inode = alloc_inode();
    if (inode)
    {
        struct inode *old;

        //spin_lock(&inode_lock);
        /* We released the lock, so.. */
        old = find_inode(sb, ino, head, find_actor, opaque);
        if (!old)
        {
            clean_inode(inode);
            
            list_add(&inode->i_list, head);
            inode->i_sb = sb;
            inode->i_dev = sb->s_dev;
            inode->i_ino = ino;
            inode->i_flags = 0;
            atomic_set(&inode->i_count, 1);
            //inode->i_state = I_LOCK;
            //spin_unlock(&inode_lock);

            if (sb->s_op && sb->s_op->read_inode)
                sb->s_op->read_inode(inode);

            /*
			 * This is special!  We do not need the spinlock
			 * when clearing I_LOCK, because we're guaranteed
			 * that nobody else tries to do anything about the
			 * state of the inode when it is locked, as we
			 * just created it (so there can be no old holders
			 * that haven't tested I_LOCK).
			 */
            //inode->i_state &= ~I_LOCK;
            //wake_up(&inode->i_wait);

            return inode;
        }

        /*
		 * Uhhuh, somebody else created the same inode under
		 * us. Use the old inode instead of the one we just
		 * allocated.
		 */
        __iget(old);
        //spin_unlock(&inode_lock);
        destroy_inode(inode);
        inode = old;
        //wait_on_inode(inode);
    }
    return inode;
}


struct inode *iget(struct super_block *sb, unsigned long ino, find_inode_t find_actor, void *arg)
{
    struct list_head *head;
    struct inode *inode;

    head = &sb->inode_list;
    //spin_lock(&sb->inode_list_lock);
    inode = find_inode(sb, ino, head, find_actor, arg);
    if (inode)
    {
        __iget(inode);
        //spin_unlock(&sb->inode_list_lock);
        //wait_on_inode(inode);`
        return inode;
    }
    //spin_unlock(&sb->inode_list_lock);

    return get_new_inode(sb, ino, head, find_actor, arg);
}