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

// Implemetation suggestion:
// -------------------------
// Use this structure as your file-object's private data structure
typedef struct {
	unsigned char key;
	int read_state;
} encdec_private_data;


char* bufcae = NULL;
char* bufxor = NULL;

int init_module(void)
{
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if(major < 0)
	{	
		return major;
	}

	// Implemetation suggestion:
	// -------------------------
	// 1. Allocate memory for the two device buffers using kmalloc (each of them should be of size 'memory_size')
	bufcae = (char*)kmalloc(memory_size, GFP_KERNEL);
	bufxor = (char*)kmalloc(memory_size, GFP_KERNEL);


	return 0;
}

void cleanup_module(void)
{
	// Implemetation suggestion:
	// -------------------------	
	// 1. Unregister the device-driver
	// 2. Free the allocated device buffers using kfree
	unregister_chrdev(major, MODULE_NAME);
	kfree(bufcae);
	kfree(bufxor);

}

int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);

	// Implemetation suggestion:
	// -------------------------
	// 1. Set 'filp->f_op' to the correct file-operations structure (use the minor value to determine which)
	// 2. Allocate memory for 'filp->private_data' as needed (using kmalloc)
	if(minor==0)
		filp->f_op = &fops_caesar;

	else if(minor==1)
		filp->f_op = &fops_xor;
	else
		return -ENODEV;

	encdec_private_data* pdata =  (encdec_private_data*) kmalloc(sizeof(encdec_private_data), GFP_KERNEL);
	pdata->key = 0;
	pdata->read_state = ENCDEC_READ_STATE_DECRYPT;
	filp->private_data = pdata;


	return 0;
}

int encdec_release(struct inode *inode, struct file *filp)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Free the allocated memory for 'filp->private_data' (using kfree)

	kfree(filp->private_data);

	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Update the relevant fields in 'filp->private_data' according to the values of 'cmd' and 'arg'

	if(MINOR(inode->i_rdev)!=0 && MINOR(inode->i_rdev)!= 1)
		return -ENODEV;
	
	switch (cmd){
		case ENCDEC_CMD_CHANGE_KEY:
			((encdec_private_data*)filp->private_data)->key = arg;
			break;
		case ENCDEC_CMD_SET_READ_STATE:
			((encdec_private_data*)filp->private_data)->read_state = arg;
			break;
		case ENCDEC_CMD_ZERO:
			if(MINOR(inode->i_rdev)==0){
				int i;
				for (i = 0; i<memory_size; i++){
					bufcae[i] = 0;
				}
			}
			else{
				int i;
				for (i = 0; i<memory_size; i++){
					bufxor[i] = 0;
				}
			}
			break;
		default:
		return -ENOTTY;

	}
	return 0;
}

// Add implementations for:
// ------------------------
// 1. ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
// 2. ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
// 3. ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
// 4. ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos ){
	if(*f_pos+count > memory_size)
		return -EINVAL;
	if(copy_to_user(buf,bufcae+*f_pos,count)!=0)
		return -EFAULT;
	if (((encdec_private_data*)filp->private_data)->read_state==ENCDEC_READ_STATE_DECRYPT){
		int i;
		for(i = 0; i<count; i++){
			buf[i] = (buf[i]+128-((encdec_private_data*)filp->private_data)->key)%128;
		}
	}
	*f_pos += count;
	return 0;
}
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos){
	if(*f_pos+count>memory_size)
		return -ENOSPC;
	if(copy_from_user(bufcae+*f_pos,buf,count)!=0)
		return -EFAULT;
	int i;
	for(i = *f_pos; i<count; i++){
		bufcae[i] = (bufcae[i]+((encdec_private_data*)filp->private_data)->key)%128;
	}
	*f_pos += count;
	return 0;
	
}
ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos ){
	if(*f_pos+count>memory_size)
		return -EINVAL;
	if(copy_to_user(buf,bufxor+*f_pos,count)!=0)
		return -EFAULT;
	if (((encdec_private_data*)filp->private_data)->read_state==ENCDEC_READ_STATE_DECRYPT){
		int i;
		for(i = 0; i<count; i++){
			buf[i] = (buf[i]^((encdec_private_data*)filp->private_data)->key);
		}
	}
	*f_pos += count;
	return 0;
}
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos){
	if(*f_pos+count>memory_size)
		return -ENOSPC;
	if(copy_from_user(bufxor+*f_pos,buf,count)!=0)
		return -EFAULT;
	int i;
	for(i = *f_pos; i<count; i++){
		bufxor[i] = (bufxor[i]^((encdec_private_data*)filp->private_data)->key);
	}
	*f_pos += count;
	return 0;
}
