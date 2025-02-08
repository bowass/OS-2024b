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
MODULE_AUTHOR("Itay Dreyfuss and Ilan Vainblat");

int 	encdec_open(struct inode *inode, struct file *filp);
int 	encdec_release(struct inode *inode, struct file *filp);
int 	encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar(struct file *filp, char *dest_buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_caesar(struct file *filp, const char *source_buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor(struct file *filp, char *dest_buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_xor(struct file *filp, const char *source_buf, size_t count, loff_t *f_pos);

int memory_size = 0;

MODULE_PARM(memory_size, "i");

int major = 0;
char *buf_ceaser, *buf_xor;

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
}encdec_private_data;

int init_module(void){
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if(major < 0)
	{
		return major;
	}

	// Implemetation suggestion:
	// -------------------------
	// 1. Allocate memory for the two device buffers using kmalloc (each of them should be of size 'memory_size')
	buf_ceaser = (char*)kmalloc(memory_size, GFP_KERNEL);
	buf_xor = (char*)kmalloc(memory_size, GFP_KERNEL);

	if(!(buf_ceaser && buf_xor)){
		return -1;
	}
	return 0;
}

void cleanup_module(void)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Unregister the device-driver
	// 2. Free the allocated device buffers using kfree
	
	int status;
	status = unregister_chrdev(major, MODULE_NAME);
	if(status < 0 ){
		//perror("error"); // in case unregister_chrdev doesn't succeed
	}
	
	kfree(buf_ceaser);
	kfree(buf_xor);
}

int encdec_open(struct inode *inode, struct file *filp)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Set 'filp->f_op' to the correct file-operations structure (use the minor value to determine which)
	// 2. Allocate memory for 'filp->private_data' as needed (using kmalloc)
	
	encdec_private_data *private_data;
	int minor = MINOR(inode->i_rdev);

	if(minor == 0)
		filp->f_op = &fops_caesar;
	else if(minor == 1)
		filp->f_op = &fops_xor;
	else 
		return -ENODEV;

	private_data = (encdec_private_data*)kmalloc(sizeof(encdec_private_data), GFP_KERNEL);
	if(private_data == NULL)
		return -ENOMEM;
	
	private_data->key = 0;
	private_data->read_state = ENCDEC_READ_STATE_DECRYPT;
	filp->private_data = private_data;
	
	return 0;
}

int encdec_release(struct inode *inode, struct file *filp)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Free the allocated memory for 'filp->private_data' (using kfree)

	kfree(filp->private_data);
	filp->private_data = NULL; //good practice
	
	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Update the relevant fields in 'filp->private_data' according to the values of 'cmd' and 'arg'
	
	int minor = MINOR(inode->i_rdev);
	encdec_private_data* private_data= (encdec_private_data*)filp->private_data;
	
	switch(cmd){
		case ENCDEC_CMD_CHANGE_KEY:
			private_data->key = arg;
			break;
		case ENCDEC_CMD_SET_READ_STATE:
			private_data->read_state = arg;
			break;
		case ENCDEC_CMD_ZERO:
			switch(minor){
				case 0:
					memset(buf_ceaser, 0, memory_size);
					break;
				case 1:
					memset(buf_ceaser, 0, memory_size);
					break;
				default:
					return -ENODEV;
			}
			break;
		default:
			return -ENOTTY;
	}
	return 0;
}


ssize_t encdec_read_caesar(struct file *filp, char *dest_buf, size_t count, loff_t *f_pos ){
	encdec_private_data* private_data= (encdec_private_data*)filp->private_data;
	int key = private_data->key, i;
	int num_not_read = 0, num_read = 0;
	
	if(*f_pos >= memory_size)
		return -EINVAL;

	if((num_not_read = copy_to_user(dest_buf, buf_ceaser + *f_pos, count)) > 0)
			return -EFAULT;
			
	num_read = count - num_not_read;
	*f_pos += num_read;
	
	if(private_data->read_state == ENCDEC_READ_STATE_DECRYPT){ //in case read state is to decrypt we need to decode the message
		for( i = 0; i< num_read; i++) //decode in ceaser cipher
			dest_buf[i] = (dest_buf[i] - key + 128) % 128;
	}

	return num_read;
}

ssize_t encdec_write_caesar(struct file *filp, const char *source_buf, size_t count, loff_t *f_pos){
	encdec_private_data* private_data = (encdec_private_data*)filp->private_data;
	int num_read = 0, num_not_read = 0, i;
	int key = private_data->key;
	
	if( *f_pos >= memory_size)
		return -ENOSPC;

	if((num_not_read = copy_from_user(buf_ceaser + *f_pos, source_buf, count)) > 0)
			return -EFAULT;
	
	num_read = count - num_not_read;
	for(i = *f_pos; i < *f_pos + num_read; i++){ //encoding in ceaser cipher
		buf_ceaser[i] = (buf_ceaser[i] + key) % 128;
	}
	
	*f_pos += num_read; 
	return num_read;
}

ssize_t encdec_read_xor(struct file *filp, char *dest_buf, size_t count, loff_t *f_pos ){
	encdec_private_data* private_data= (encdec_private_data*)filp->private_data;
	int key = private_data->key, i;
	int num_read = 0, num_not_read = 0;

	if(*f_pos >= memory_size) 
		return -EINVAL;

	if((num_not_read = copy_to_user(dest_buf, buf_xor + *f_pos, count)) > 0)
			return -EFAULT;
	
	num_read = count - num_not_read;
	*f_pos += num_read; 
	
	if(private_data->read_state == ENCDEC_READ_STATE_DECRYPT){ //in case read state is to decrypt we need to decrypt the message
		for(i = 0; i< num_read; i++){ //decode in xor cipher
			dest_buf[i] = dest_buf[i] ^ key;
		}
	}

	return num_read; 
}

ssize_t encdec_write_xor(struct file *filp, const char *source_buf, size_t count, loff_t *f_pos){
	encdec_private_data* private_data = (encdec_private_data*)filp->private_data;
	int i = 0, num_read = 0, num_not_read = 0;
	int key = private_data->key;
	
	if(*f_pos >= memory_size) 
		return -ENOSPC;
	
	if((num_not_read = copy_from_user(buf_xor + *f_pos, source_buf, count)) > 0)
			return -EFAULT;
			
	num_read = count - num_not_read;
	for(i = *f_pos; i < *f_pos + num_read; i++){ //encoding in xor cipher
		buf_xor[i] = buf_xor[i] ^ key;
	}
	
	*f_pos += num_read;
	return num_read;
}
