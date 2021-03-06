#ifndef __CHAR_DEV_H__
#define __CHAR_DEV_H__

struct chr_dev_struct 
{
	int dev;
	const char *name;
	struct file_operations *fops;
	void *private_data;
};

#define NR_CHRDEV   MAX_CHRDEV

extern int register_chrdev(unsigned int major, char * name, struct file_operations *fops);
extern void unregister_chrdev(unsigned int major);
extern void *get_cdev_private_data(int major);
extern int set_cdev_private_data(int major, void *data);

#endif


