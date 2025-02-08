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

char* buffer_caesar;
char* buffer_xor;

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
} encdec_private_date;



int init_module(void)
{
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if(major < 0)
	{
		return major;
	}
	buffer_caesar = (char*)kmalloc(memory_size, GFP_KERNEL);
	buffer_xor = (char*)kmalloc(memory_size, GFP_KERNEL);

	// Implemetation suggestion:
	// -------------------------
	// 1. Allocate memory for the two device buffers using kmalloc (each of them should be of size 'memory_size')

	return 0;
}

void cleanup_module(void)
{
	major = unregister_chrdev(major, MODULE_NAME);
	kfree(buffer_caesar);
	kfree(buffer_xor);
	// Implemetation suggestion:
	// -------------------------
	// 1. Unregister the device-driver
	// 2. Free the allocated device buffers using kfree
}

//zero  buffer
void zero_buffer(struct file* filp)
{
	if (filp->f_op == &fops_caesar)
		memset(buffer_caesar, 0, memory_size);
	if (filp->f_op == &fops_xor)
		memset(buffer_xor, 0, memory_size);
}

int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);
	filp->private_data = (encdec_private_date*)kmalloc(sizeof(encdec_private_date), GFP_KERNEL);
	encdec_private_date* x = (encdec_private_date*)filp->private_data;
	if(minor==0)
    {
		filp->f_op = &fops_caesar;
		x->key = ENCDEC_CMD_CHANGE_KEY;
		x->read_state = ENCDEC_READ_STATE_DECRYPT;
    }
	if (minor == 1)
	{
		filp->f_op = &fops_xor;
		x->key = ENCDEC_CMD_CHANGE_KEY;
		x->read_state = ENCDEC_READ_STATE_DECRYPT;
	}

	// Implemetation suggestion:
	// -------------------------
	// 1. Set 'filp->f_op' to the correct file-operations structure (use the minor value to determine which)
	// 2. Allocate memory for 'filp->private_data' as needed (using kmalloc)

	return 0;
}

int encdec_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	// Implemetation suggestion:
	// -------------------------
	// 1. Free the allocated memory for 'filp->private_data' (using kfree)

	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{

	if (!filp->private_data)
		return -EINVAL;
	encdec_private_date* x=((encdec_private_date*)filp->private_data);
	if (cmd == ENCDEC_CMD_CHANGE_KEY)
		 x->key= arg;
	if (cmd== ENCDEC_CMD_SET_READ_STATE)
		x->read_state= arg;
	if (cmd == ENCDEC_CMD_ZERO)
	{
		zero_buffer(filp);
	}
	// Implemetation suggestion:
	// -------------------------
	// 1. Update the relevant fields in 'filp->private_data' according to the values of 'cmd' and 'arg'

	return 0;
}

// Add implementations for:
// ------------------------
ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos )
{
	int i, new_count, y;
	encdec_private_date* x = ((encdec_private_date*)filp->private_data);
	
	if (*f_pos == memory_size)
		return -EINVAL;
	if ((*f_pos) + count > memory_size)
		new_count = memory_size - (*f_pos);
	else
		new_count = count;
	//decode
	if (x->read_state == ENCDEC_READ_STATE_DECRYPT) {
		for (i = *f_pos;i < new_count + (*f_pos);i++) {
			buffer_caesar[i] = (buffer_caesar[i] + 128 - x->key) % 128;
		}
	}
	y=copy_to_user(buf,buffer_caesar+(*f_pos), new_count);
	//encode
	if (x->read_state == ENCDEC_READ_STATE_DECRYPT) {
		for (i = *f_pos;i < new_count + (*f_pos);i++) {
			buffer_caesar[i] = (buffer_caesar[i] + x->key) % 128;
		}
	}
	if (y != 0)
		return -EFAULT;
	*f_pos += new_count;

	return new_count;
}
ssize_t encdec_write_caesar(struct file* filp, const char* buf, size_t count, loff_t* f_pos)
{
	int i, new_count, y;
	encdec_private_date* x = (encdec_private_date*)filp->private_data;
	if (*f_pos == memory_size)
		return -ENOSPC;
	if (*f_pos + count > memory_size)
		new_count = memory_size - (*f_pos);
	else
		new_count = count;
	y = copy_from_user(buffer_caesar + (*f_pos), buf, new_count);
	if (y != 0)
		return -EFAULT;
	//encode
	for (i = *f_pos;i < new_count+(*f_pos);i++)
		buffer_caesar[i] = (buffer_caesar[i] + x->key) % 128;

	*f_pos += new_count;
	return new_count;
}
ssize_t encdec_read_xor(struct file* filp, char* buf, size_t count, loff_t* f_pos)
{
	int i, new_count, y;
	encdec_private_date* x = ((encdec_private_date*)filp->private_data);

	if (*f_pos == memory_size)
		return -EINVAL;
	if ((*f_pos) + count > memory_size)
		new_count = memory_size - (*f_pos);
	else
		new_count = count;
	//decode
	if (x->read_state == ENCDEC_READ_STATE_DECRYPT) {
		for (i = *f_pos;i < new_count + (*f_pos);i++) {
			buffer_xor[i] = buffer_xor[i] ^ x->key;
		}
	}
	y = copy_to_user(buf, buffer_xor+(*f_pos), new_count);
	//encode
	if (x->read_state == ENCDEC_READ_STATE_DECRYPT) {
		for (i = *f_pos;i < new_count+(*f_pos);i++) {
			buffer_xor[i] = buffer_xor[i] ^ x->key;
		}
	}
	if (y != 0)
		return -EFAULT;
	*f_pos += new_count;
	return new_count;
}
ssize_t encdec_write_xor(struct file* filp, const char* buf, size_t count, loff_t* f_pos)
{
	int i,new_count,y;
	encdec_private_date* x = (encdec_private_date*)filp->private_data;
	if (*f_pos == memory_size)
		return -ENOSPC;
	if (*f_pos + count > memory_size)
		new_count = memory_size - (*f_pos);
	else
		new_count = count;
	y=copy_from_user(buffer_xor+(*f_pos), buf, new_count);
	//encode
	for (i = *f_pos;i < new_count+(*f_pos);i++)
		buffer_xor[i] = buffer_xor[i] ^ x->key;
	if (y != 0)
		return -EFAULT;
	*f_pos += new_count;
	return new_count;
}
