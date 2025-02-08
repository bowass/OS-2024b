#include <linux/ctype.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>  	
#include <linux/slab.h>
#include <linux/fs.h>       		
#include <linux/errno.h>  
#include <linux/types.h> 
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/string.h>

#include "encdec.h"

#define MODULE_NAME "encdec"
#define CAESAR_MINOR 0
#define XOR_MINOR 1

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YOUR NAME");

int 	encdec_open(struct inode *inode, struct file *filp);
int 	encdec_release(struct inode *inode, struct file *filp);
int 	encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

int memory_size = 0;

MODULE_PARM(memory_size, "i");

int major = 0;

struct file_operations fops_caesar = {
	.open 	 =	encdec_open,
	.release =	encdec_release,
	.read 	 =	encdec_read_caesar,
	.write 	 =	encdec_write_caesar,
	.llseek  =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
};

struct file_operations fops_xor = {
	.open 	 =	encdec_open,
	.release =	encdec_release,
	.read 	 =	encdec_read_xor,
	.write 	 =	encdec_write_xor,
	.llseek  =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
};


typedef struct {
	unsigned char key;
	int read_state;
} encdec_private_data;


char *caesar_buf;
char *xor_buf;


int init_module(void)
{
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if(major < 0)
	{	
		return major;
	}

	caesar_buf = (char*)kmalloc(sizeof(char) * memory_size, GFP_KERNEL);
	xor_buf = (char*)kmalloc(sizeof(char) * memory_size, GFP_KERNEL);


	return 0;
}

void cleanup_module(void)
{
	unregister_chrdev(major, MODULE_NAME);

	kfree(caesar_buf);
	kfree(xor_buf);

}

int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);


	if (minor == CAESAR_MINOR)
		filp->f_op = &fops_caesar;
	else if (minor == XOR_MINOR)
		filp->f_op = &fops_xor;
	else
		return -ENODEV; 

	encdec_private_data* p;

	p = (encdec_private_data*)kmalloc(sizeof(encdec_private_data), GFP_KERNEL);
	if (!p)
		return -1;

	p->key = 0;
	p->read_state = ENCDEC_READ_STATE_DECRYPT;
	filp->private_data = p;

	return 0;
}

int encdec_release(struct inode *inode, struct file *filp)
{
	

	kfree(filp->private_data);

	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	
	int minor = MINOR(inode->i_rdev);

	encdec_private_data* p;
	p = (encdec_private_data*)filp->private_data;

	switch (cmd) {
		case ENCDEC_CMD_CHANGE_KEY:
			p->key = arg; 
			break;

		case ENCDEC_CMD_SET_READ_STATE :
			p->read_state = arg;
			break;

		case ENCDEC_CMD_ZERO:

				if (minor == CAESAR_MINOR)
					memset(caesar_buf, 0, memory_size);

				else if (minor == XOR_MINOR)
					memset(xor_buf, 0, memory_size);
				else
					return -ENODEV;
			
				break;

		default:
			return -ENOTTY;
	}

	return 0;
}



ssize_t encdec_write_caesar(struct file* filp, const char* buf, size_t count, loff_t* f_pos)
{

	int i, diff = count;

	encdec_private_data* p = (encdec_private_data*)(filp->private_data);
	
	unsigned char k = p->key;

	if (*f_pos == memory_size)
		return -ENOSPC;

	if (*f_pos + count >= memory_size)
		diff = memory_size - *f_pos;
	

	if (copy_from_user(caesar_buf+(*f_pos), buf, diff) != 0)
		return -EFAULT;

	for(i = 0; i < diff; i++)
	{
		
		caesar_buf[(*f_pos) + i] = (caesar_buf[(*f_pos) + i] + k) % 128;
	
	}

	*f_pos += (loff_t)(diff * sizeof(char));

	printk("testing write: %d\n", (int)*f_pos);

	return diff;

}

ssize_t encdec_read_caesar(struct file* filp, char* buf, size_t count, loff_t* f_pos)
{
	encdec_private_data* p = (encdec_private_data*)(filp->private_data);

	int i, diff = count, state = p->read_state;

	unsigned char k = p->key;

	if (*f_pos == memory_size)
		return -EINVAL;

	if (*f_pos + count >= memory_size)
		diff = memory_size - *f_pos;

	if (copy_to_user(buf, caesar_buf + (*f_pos), diff) != 0)
		return -EFAULT;

	if(state == ENCDEC_READ_STATE_DECRYPT)
	{
		for(i = 0; i < diff; i++)
			buf[i] = (buf[i] + 128 - k) % 128;
	}

	*f_pos += (loff_t)(diff * sizeof(char));

	return diff;
}



ssize_t encdec_write_xor(struct file* filp, const char* buf, size_t count, loff_t* f_pos)
{

	int i, diff = count;

	encdec_private_data* p = (encdec_private_data*)(filp->private_data);

	unsigned char k = p->key;

	if (*f_pos == memory_size)
		return -ENOSPC;

	if (*f_pos + count >= memory_size)
		diff = memory_size - *f_pos;



	if (copy_from_user(xor_buf + (*f_pos), buf, diff) != 0)
		return -EFAULT;

	for (i = 0; i < diff; i++)
	{

		xor_buf[(*f_pos) + i] = xor_buf[(*f_pos) + i] ^ k;

	}

	*f_pos += (loff_t)(diff * sizeof(char));

	return diff;

}

ssize_t encdec_read_xor(struct file* filp, char* buf, size_t count, loff_t* f_pos)
{

	encdec_private_data* p = (encdec_private_data*)(filp->private_data);

	int i, diff = count, state = p->read_state;

	unsigned char k = p->key;

	if (*f_pos == memory_size)
		return -EINVAL;

	if (*f_pos + count >= memory_size)
		diff = memory_size - *f_pos;

	if (copy_to_user(buf, xor_buf + (*f_pos), diff) != 0)
		return -EFAULT;

	if (state == ENCDEC_READ_STATE_DECRYPT)
	{
		for (i = 0; i < diff; i++)
			buf[i] = buf[i] ^ k;
	}

	*f_pos += (loff_t)(diff * sizeof(char));

	return diff;

}
