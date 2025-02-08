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
MODULE_AUTHOR("Shmuel Oded Brandt");

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
	.release 	 =	encdec_release,
	.read 	 =	encdec_read_caesar,
	.write 	 =	encdec_write_caesar,
	.llseek  	 =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
};

struct file_operations fops_xor = {
	.open 	 =	encdec_open,
	.release 	 =	encdec_release,
	.read 	 =	encdec_read_xor,
	.write 	 =	encdec_write_xor,
	.llseek  	 =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
};


// -------------------------

typedef struct {
	unsigned char key;
	int read_state;
} encdec_private_data;

char *caesar_buf, *xor_buf;

int init_module(void)
{
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if(major < 0)
		return major;

	caesar_buf = (char*) kmalloc(sizeof(char)*memory_size, GFP_KERNEL);
	xor_buf = (char*) kmalloc(sizeof(char)*memory_size, GFP_KERNEL);

	memset(caesar_buf, 0, memory_size);
	memset(xor_buf, 0, memory_size);

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
	
	if (minor == 0) { // not needed b\c its the defaul value from init_module?
		filp->f_op = &fops_caesar;
	}
	else if (minor == 1) {
		filp->f_op = &fops_xor;
	}
	else {
		return -ENODEV;
	}
	
	encdec_private_data *p_data = (encdec_private_data*) kmalloc(sizeof(encdec_private_data), GFP_KERNEL);

	p_data->key = 0;
	p_data->read_state = ENCDEC_READ_STATE_DECRYPT;
	
	filp->private_data = p_data;

	return 0;
}

int encdec_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);

	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	if (cmd == ENCDEC_CMD_CHANGE_KEY) {
		encdec_private_data *p_data = (encdec_private_data*)filp->private_data;
		p_data->key = arg;
	}
	else if (cmd == ENCDEC_CMD_SET_READ_STATE) {
		encdec_private_data *p_data = (encdec_private_data*)filp->private_data;
		p_data->read_state = arg;
	}
	else if (cmd == ENCDEC_CMD_ZERO) {
		int minor = MINOR(inode->i_rdev);
		if (minor == 0) {
			memset(caesar_buf, 0, memory_size);
		}
		else if (minor == 1) {
			memset(xor_buf, 0, memory_size);
		}
		else {
			return -ENODEV;
		}
	}
	else {
		return -1; // not supposed to get here
	}

	return 0;
}

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	if (*f_pos == memory_size)
		return -ENOSPC;

	// recive data
	size_t amount = min(count, (size_t)(memory_size - *f_pos)); // how much to copy
	if (copy_from_user(caesar_buf + *f_pos, buf, amount) != 0)
		return -EFAULT;
	
	// encode message
	encdec_private_data *p_data = filp->private_data;
	unsigned char key = p_data->key;
	unsigned long long i;
	for (i = 0; i < amount; i++)
		caesar_buf[i+*f_pos] = (caesar_buf[i+*f_pos] + key) % 128;

	*f_pos += amount; // sizeof(char) is 1 byte so the conversion is easy
	
	return amount;
}

ssize_t encdec_read_caesar(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
	if (*f_pos == memory_size)
		return -EINVAL;
	
	// temporery biffer store (maybe decrypted) data
	char *temp = (char*) kmalloc(sizeof(char)*memory_size, GFP_KERNEL);

	int j;
	for (j = 0; j < memory_size; j++)
		temp[j] = caesar_buf[j];
	
	// decrypt if needed
	encdec_private_data *p_data = filp->private_data;
	if (p_data->read_state == ENCDEC_READ_STATE_DECRYPT) {
		unsigned char key = p_data->key;
		unsigned long long i;
		for (i = 0; i+*f_pos < memory_size; i++)
			temp[i+*f_pos] = (temp[i+*f_pos] + 128 - key) % 128;
	}

	// send data
	size_t amount = min(count, (size_t)(memory_size - *f_pos)); // how much to copy
	if (copy_to_user(buf, temp + *f_pos, amount) != 0)
		return -EFAULT;

	*f_pos += amount;

	// free up temp
	kfree(temp);

	return amount;
}

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	if (*f_pos == memory_size)
		return -ENOSPC;

	// recive data
	size_t amount = min(count, (size_t)(memory_size - *f_pos)); // how much to copy
	if (copy_from_user(xor_buf + *f_pos, buf, amount) != 0)
		return -EFAULT;
	
	// encode message
	encdec_private_data *p_data = filp->private_data;
	unsigned char key = p_data->key;
	unsigned long long i;
	for (i = 0; i < amount; i++)
		xor_buf[i+*f_pos] = (xor_buf[i+*f_pos] ^ key);

	*f_pos += amount; // sizeof(char) is 1 byte so the conversion is easy
	
	return amount;
}

ssize_t encdec_read_xor(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
	if (*f_pos == memory_size)
		return -EINVAL;
	
	// temporery biffer store (maybe decrypted) data
	char *temp = (char*) kmalloc(sizeof(char)*memory_size, GFP_KERNEL);

	int j;
	for (j = 0; j < memory_size; j++)
		temp[j] = xor_buf[j];
	
	// decrypt if needed
	encdec_private_data *p_data = filp->private_data;
	if (p_data->read_state == ENCDEC_READ_STATE_DECRYPT) {
		unsigned char key = p_data->key;
		unsigned long long i;
		for (i = 0; i+*f_pos < memory_size; i++)
			temp[i+*f_pos] = (temp[i+*f_pos] ^ key);
	}

	// send data
	size_t amount = min(count, (size_t)(memory_size - *f_pos)); // how much to copy
	if (copy_to_user(buf, temp + *f_pos, amount) != 0)
		return -EFAULT;

	*f_pos += amount;

	// free up temp
	kfree(temp);

	return amount;
}
