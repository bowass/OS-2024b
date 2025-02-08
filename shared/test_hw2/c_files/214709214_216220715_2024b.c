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

int encdec_open(struct inode *inode, struct file *filp);
int encdec_release(struct inode *inode, struct file *filp);
int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

char *allocateMem();
void freeMem();
void free_private_data(struct file *);
void ceasar_encode(struct file *filp, char *s, size_t count, int f_pos);
void ceasar_decode(struct file *filp, char *s, size_t count, int f_pos);
void xor_encode(struct file *filp, char *s, size_t count, int f_pos);
void xor_decode(struct file *filp, char *s, size_t count, int f_pos);
int memory_size = 0;

MODULE_PARM(memory_size, "i");

int major = 0;

char *ceasar_buffer;
char *xor_buffer;

struct file_operations fops_caesar = {
	.open = encdec_open,
	.release = encdec_release,
	.read = encdec_read_caesar,
	.write = encdec_write_caesar,
	.llseek = NULL,
	.ioctl = encdec_ioctl,
	.owner = THIS_MODULE};

struct file_operations fops_xor = {
	.open = encdec_open,
	.release = encdec_release,
	.read = encdec_read_xor,
	.write = encdec_write_xor,
	.llseek = NULL,
	.ioctl = encdec_ioctl,
	.owner = THIS_MODULE};

// Implemetation suggestion:
// -------------------------
// Use this structure as your file-object's private data structure
typedef struct
{
	unsigned char key;
	int read_state;
} encdec_private_date;

encdec_private_date *allocate_private_data();

char *allocateMem()
{
	return ((char *)kmalloc(memory_size * sizeof(char), GFP_KERNEL));
}
void freeMem()
{
	kfree(xor_buffer);
	kfree(ceasar_buffer);
	return;
}
int init_module(void)
{
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if (major < 0)
	{
		return major;
	}

	// Implemetation suggestion:
	// -------------------------
	// 1. Allocate memory for the two device buffers using kmalloc (each of them should be of size 'memory_size')
	ceasar_buffer = allocateMem();
	xor_buffer = allocateMem();
	return 0;
}
void cleanup_module(void)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Unregister the device-driver
	// 2. Free the allocated device buffers using kfree
	unregister_chrdev(major, MODULE_NAME);
	freeMem();
	return;
}
encdec_private_date *allocate_private_data()
{
	encdec_private_date *my_private_data = (encdec_private_date *)kmalloc(sizeof(encdec_private_date), GFP_KERNEL);
	return my_private_data;
}
int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);

	// Implemetation suggestion:
	// -------------------------
	// 1. Set 'filp->f_op' to the correct file-operations structure (use the minor value to determine which)
	// 2. Allocate memory for 'filp->private_data' as needed (using kmalloc)
	if (minor == 0)
	{
		filp->f_op = &fops_caesar;
	}
	else if (minor == 1)
	{
		filp->f_op = &fops_xor;
	}
	else
	{
		return -ENODEV;
	}
	filp->private_data = allocate_private_data();
	encdec_ioctl(inode, filp, ENCDEC_CMD_CHANGE_KEY, 0);
	encdec_ioctl(inode, filp, ENCDEC_CMD_SET_READ_STATE, ENCDEC_READ_STATE_DECRYPT);
	return 0;
}

void free_private_data(struct file *filp)
{
	kfree(filp->private_data);
	return;
}

int encdec_release(struct inode *inode, struct file *filp)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Free the allocated memory for 'filp->private_data' (using kfree)
	free_private_data(filp);
	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Update the relevant fields in 'filp->private_data' according to the values of 'cmd' and 'arg'
	int minor = MINOR(inode->i_rdev);
	if (cmd == ENCDEC_CMD_CHANGE_KEY)
	{
		((encdec_private_date *)filp->private_data)->key = arg;
	}
	else if (cmd == ENCDEC_CMD_SET_READ_STATE)
	{
		if (arg == ENCDEC_READ_STATE_DECRYPT)
		{
			((encdec_private_date *)filp->private_data)->read_state = ENCDEC_READ_STATE_DECRYPT;
		}
		else if (arg == ENCDEC_READ_STATE_RAW)
		{
			((encdec_private_date *)filp->private_data)->read_state = ENCDEC_READ_STATE_RAW;
		}
	}
	else if (cmd == ENCDEC_CMD_ZERO)
	{
		if (minor == 0)
		{
			memset(ceasar_buffer, 0, memory_size);
		}
		else if (minor == 1)
		{
			memset(xor_buffer, 0, memory_size);
		}
		else 
		{
			return -ENODEV;
		}
	}
	else
	{
		return -1;
	}
	return 0;
}

// Add implementations for:
// ------------------------
// 1. ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
// 2. ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
// 3. ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
// 4. ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

void ceasar_encode(struct file *filp, char *s, size_t count, int f_pos)
{
	int i = 0;
	encdec_private_date *private = (encdec_private_date *)filp->private_data;
	while ((count != i) && (f_pos + i != memory_size))
	{
		s[f_pos + i] = (s[f_pos + i] + private->key) % 128;
		i++;
	}
	return;
}
void ceasar_decode(struct file *filp, char *s, size_t count, int f_pos)
{
	int i = 0;
	encdec_private_date *private = (encdec_private_date *)filp->private_data;
	while ((count != i) && (f_pos + i != memory_size))
	{
		s[f_pos + i] = (s[f_pos + i] + 128 - private->key) % 128;
		i++;
	}
	return;
}

void xor_encode(struct file *filp, char *s, size_t count, int f_pos)
{
	int i = 0;
	encdec_private_date *private = (encdec_private_date *)filp->private_data;

	while ((count != i) && (f_pos + i != memory_size))
	{
		s[f_pos + i] = s[f_pos + i] ^ (private->key);
		i++;
	}
	return;
}

void xor_decode(struct file *filp, char *s, size_t count, int f_pos)
{
	int i = 0;
	encdec_private_date *private = (encdec_private_date *)filp->private_data;

	while ((count != i) && (f_pos + i != memory_size))
	{
		s[f_pos + i] = s[f_pos + i] ^ (private->key);
		i++;
	}
	return;
}

ssize_t encdec_read_caesar(struct file *filp, char *buf, size_t count, loff_t *f_pos) // del the encode
{
	encdec_private_date *private = (encdec_private_date *)filp->private_data;
	int x = 0;
	if (private->read_state == ENCDEC_READ_STATE_RAW)
	{
		if (*(f_pos) == memory_size)
		{
			return -EINVAL;
		}
		else if (*f_pos + count > memory_size)
		{
			if (copy_to_user(buf, ceasar_buffer + (*f_pos), memory_size - (*f_pos)) != 0) // buf user
			{
				return -EFAULT;
			}
			x = *f_pos;
			*f_pos = memory_size;
			return memory_size - x;
		}
		else
		{
			if (copy_to_user(buf, ceasar_buffer + (*f_pos), count) != 0) // buf user
			{
				return -EFAULT;
			}
			*f_pos = *f_pos + count;
			return count;
		}
	}
	else if (private->read_state == ENCDEC_READ_STATE_DECRYPT)
	{
		if (*(f_pos) == memory_size)
		{
			return -EINVAL;
		}
		else if (*f_pos + count > memory_size)
		{
			ceasar_decode(filp, ceasar_buffer, count, *f_pos);
			if (copy_to_user(buf, ceasar_buffer + (*f_pos), memory_size - (*f_pos)) != 0) // buf user
			{
				return -EFAULT;
			}
			ceasar_encode(filp, ceasar_buffer, count, *f_pos);
			x = *f_pos;
			*f_pos = memory_size;

			return memory_size - x;
		}
		else
		{
			ceasar_decode(filp, ceasar_buffer, count, *f_pos);
			if (copy_to_user(buf, ceasar_buffer + (*f_pos), count) != 0) // buf user
			{
				return -EFAULT;
			}
			ceasar_encode(filp, ceasar_buffer, count, *f_pos);
			*f_pos = *f_pos + count;

			return count;
		}
	}
	return -1;
}

ssize_t encdec_read_xor(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	encdec_private_date *private = (encdec_private_date *)filp->private_data;
	int x = 0;
	if (private->read_state == ENCDEC_READ_STATE_RAW)
	{
		if (*(f_pos) == memory_size)
		{
			return -EINVAL;
		}
		else if (*f_pos + count > memory_size)
		{
			if (copy_to_user(buf, xor_buffer + (*f_pos), memory_size - (*f_pos)) != 0) // buf user
			{
				return -EFAULT;
			}
			x = *f_pos;
			*f_pos = memory_size;
			return memory_size - x;
		}
		else
		{
			if (copy_to_user(buf, xor_buffer + (*f_pos), count) != 0) // buf user
			{
				return -EFAULT;
			}
			*f_pos = *f_pos + count;
			return count;
		}
	}
	else if (private->read_state == ENCDEC_READ_STATE_DECRYPT)
	{

		if (*(f_pos) == memory_size)
		{
			return -EINVAL;
		}
		else if (*f_pos + count > memory_size)
		{
			xor_decode(filp, xor_buffer, count, *f_pos);
			if (copy_to_user(buf, xor_buffer + (*f_pos), memory_size - (*f_pos)) != 0) // buf user
			{
				return -EFAULT;
			}
			xor_encode(filp, xor_buffer, count, *f_pos);
			x = *f_pos;
			*f_pos = memory_size;

			return memory_size - x;
		}
		else
		{
			xor_decode(filp, xor_buffer, count, *f_pos);
			if (copy_to_user(buf, xor_buffer + (*f_pos), count) != 0) // buf user
			{
				return -EFAULT;
			}
			xor_encode(filp, xor_buffer, count, *f_pos);
			*f_pos = *f_pos + count;

			return count;
		}
	}
	return -1;
}

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int x = 0;
	if (*f_pos == memory_size)
	{
		return -ENOSPC;
	}
	else if ((*f_pos) + count > memory_size)
	{
		if (copy_from_user(ceasar_buffer + (*f_pos), buf, memory_size - (*f_pos)) != 0)
		{
			return -EFAULT;
		}
		ceasar_encode(filp, ceasar_buffer, count, *f_pos);
		x = *f_pos;
		*f_pos = memory_size;

		return memory_size - x;
	}
	else
	{
		if ((copy_from_user(ceasar_buffer + (*f_pos), buf, count) != 0))
		{
			return -EFAULT;
		}
		ceasar_encode(filp, ceasar_buffer, count, *f_pos);
		*f_pos = *f_pos + count;

		return count;
	}
}

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int x = 0;
	if (*f_pos == memory_size)
	{
		return -ENOSPC;
	}
	else if ((*f_pos) + count > memory_size)
	{
		if (copy_from_user(xor_buffer + (*f_pos), buf, memory_size - (*f_pos)) != 0)
		{
			return -EFAULT;
		}
		xor_encode(filp, xor_buffer, count, *f_pos);
		x = *f_pos;
		*f_pos = memory_size;
		return memory_size - x;
	}
	else
	{
		if ((copy_from_user(xor_buffer + (*f_pos), buf, count) != 0))
		{
			return -EFAULT;
		}
		xor_encode(filp, xor_buffer, count, *f_pos);
		*f_pos = *f_pos + count;

		return count;
	}
}
