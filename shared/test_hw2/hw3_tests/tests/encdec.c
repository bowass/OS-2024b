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

#define MODULE_NAME 	"encdec"
#define DEVICE_COUNT 	2

#define CAESAR_DEVICE 	0
#define XOR_DEVICE 		1

#define DECRYPT_CAESAR 	0
#define DECRYPT_XOR 	1
#define DECRYPT_RAW 	2

#define ENCRYPT_CAESAR 	0
#define ENCRYPT_XOR 	1

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roy Velich");

int 	encdec_open(struct inode *inode, struct file *filp);
int 	encdec_release(struct inode *inode, struct file *filp);
int 	encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_encrypt(struct file *filp, const char *buf, size_t count, loff_t *f_pos, int type, int device);
ssize_t encdec_decrypt(struct file *filp, char *buf, size_t count, loff_t *f_pos, int type, int device);

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
	char* data;
} encdec_device;

typedef struct {
	unsigned char key;
	int read_state;
} encdec_private_date;

encdec_device* devices;
char* temp_buffer;

int init_module(void)
{
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if(major < 0)
	{	
		return major;
	}

	devices = (encdec_device*)kmalloc(DEVICE_COUNT * sizeof(encdec_device), GFP_KERNEL);

	int i;
	for(i = 0; i < DEVICE_COUNT; i++)
	{
		devices[i].data = (char*)kmalloc(memory_size * sizeof(char), GFP_KERNEL);
		memset(devices[i].data, 0, memory_size * sizeof(char));
	}

	temp_buffer = (char*)kmalloc(memory_size * sizeof(char), GFP_KERNEL);

	return 0;
}

void cleanup_module(void)
{
	unregister_chrdev(major, MODULE_NAME);

	int i;
	for(i = 0; i < DEVICE_COUNT; i++)
	{
		kfree(devices[i].data);
	}

	kfree(devices);
	kfree(temp_buffer);
}

int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);
	if (minor >= DEVICE_COUNT)
	{
		return -ENODEV;
	}

	if(minor == XOR_DEVICE)
	{
		filp->f_op = &fops_xor;
	}

	filp->private_data = (encdec_private_date*)kmalloc(sizeof(encdec_private_date), GFP_KERNEL);

	encdec_private_date* private_date = filp->private_data;
	private_date->read_state = ENCDEC_READ_STATE_DECRYPT;
	private_date->key = 0;

	return 0;
}

int encdec_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

ssize_t encdec_encrypt(struct file *filp, const char *buf, size_t count, loff_t *f_pos, int type, int device)
{
	if(count <= (memory_size - *f_pos))
	{
		if(copy_from_user((void*)devices[device].data + *f_pos, (const void*)buf, count) > 0)
		{
			return -EFAULT;
		}

		encdec_private_date* private_date = filp->private_data;
		int i;
		int before;
		for(i = *f_pos; i < *f_pos + count; i++)
		{
			switch(type)
			{
				case ENCRYPT_CAESAR:
					before = devices[device].data[i];
					devices[device].data[i] = (devices[device].data[i] + private_date->key) % 128;
					break;
				case ENCRYPT_XOR:
					devices[device].data[i] = devices[device].data[i] ^ private_date->key;
					break;				
			}
		}

		*f_pos += count;
		return count;
	}
	
	return -ENOSPC;	
}

ssize_t encdec_decrypt(struct file *filp, char *buf, size_t count, loff_t *f_pos, int type, int device)
{
	if(*f_pos == memory_size)
	{
		return -EINVAL;
	}

	int available = memory_size - *f_pos;
	if(count > available)
	{
		count = available;
	}

	memset(temp_buffer, 0, memory_size);
	memcpy(temp_buffer, (void*)devices[device].data + *f_pos, count);

	encdec_private_date* private_date = filp->private_data;
	int i;
	for(i = 0; i < count; i++)
	{
		switch(type)
		{
			case DECRYPT_RAW:
				temp_buffer[i] = temp_buffer[i];
				break;
			case DECRYPT_CAESAR:
				temp_buffer[i] = ((temp_buffer[i] - private_date->key) + 128) % 128;
				break;
			case DECRYPT_XOR:
				temp_buffer[i] = temp_buffer[i] ^ private_date->key;
				break;				
		}
	}

	if(copy_to_user((void*)buf, (void*)temp_buffer, count) > 0)
	{
		return -EFAULT;
	}

	*f_pos += count;
	return count;	
}

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	return encdec_encrypt(filp, buf, count, f_pos, ENCRYPT_CAESAR, CAESAR_DEVICE);
}

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	return encdec_encrypt(filp, buf, count, f_pos, ENCRYPT_XOR, XOR_DEVICE);
}

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	encdec_private_date* private_date = filp->private_data;
	int decrypt_type = DECRYPT_XOR;
	if(private_date->read_state == ENCDEC_READ_STATE_RAW)
	{
		decrypt_type = DECRYPT_RAW;
	}

	return encdec_decrypt(filp, buf, count, f_pos, decrypt_type, XOR_DEVICE);
}

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	encdec_private_date* private_date = filp->private_data;
	int decrypt_type = DECRYPT_CAESAR;
	if(private_date->read_state == ENCDEC_READ_STATE_RAW)
	{
		decrypt_type = DECRYPT_RAW;
	}

	return encdec_decrypt(filp, buf, count, f_pos, decrypt_type, CAESAR_DEVICE);
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	encdec_private_date* private_date = filp->private_data;
	int minor;
	switch(cmd)
	{
		case ENCDEC_CMD_CHANGE_KEY:
			private_date->key = arg;
			break;
		case ENCDEC_CMD_SET_READ_STATE:
			private_date->read_state = arg;
			break;
		case ENCDEC_CMD_ZERO:
			minor = MINOR(inode->i_rdev);
			if (minor >= DEVICE_COUNT)
			{
				return -ENODEV;
			}
			memset(devices[MINOR(inode->i_rdev)].data, 0, memory_size * sizeof(char));
			break;	

	}

	return 0;
}
