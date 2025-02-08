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
MODULE_AUTHOR("Yaniv and Eden");

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

static char *device_buffer[2];

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


	device_buffer[0] = (char *)kmalloc(memory_size, GFP_KERNEL);
	if (!device_buffer[0]) {
		printk(KERN_ERR "Unable to allocate memory for buffer1.\n");
		return -ENOMEM;
	}

	device_buffer[1] = (char *)kmalloc(memory_size, GFP_KERNEL);
	if (!device_buffer[1]) {
		printk(KERN_ERR "Unable to allocate memory for buffer1.\n");
		return -ENOMEM;
	}

	memset(device_buffer[0], 0, memory_size);
	memset(device_buffer[1], 0, memory_size);




	return 0;
}

void cleanup_module(void)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Unregister the device-driver
	// 2. Free the allocated device buffers using kfree


	unregister_chrdev(major, MODULE_NAME);
	kfree(device_buffer[0]);
	kfree(device_buffer[1]);

}

int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);

	// Implemetation suggestion:
	// -------------------------
	// 1. Set 'filp->f_op' to the correct file-operations structure (use the minor value to determine which)
	// 2. Allocate memory for 'filp->private_data' as needed (using kmalloc)


	if (minor == 0) {
		filp->f_op = &fops_caesar;
	} else if (minor == 1) {
		filp->f_op = &fops_xor;
	} else {
		return -ENODEV;
	}

	encdec_private_data *private_data = (encdec_private_data *)kmalloc(sizeof(encdec_private_data), GFP_KERNEL);
    if (!private_data) {
        return -ENOMEM;
    }

	private_data->key = 0;
	private_data->read_state = ENCDEC_READ_STATE_RAW;

	filp->private_data = private_data;




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

	int minor = MINOR(inode->i_rdev);
	encdec_private_data * private_data = (encdec_private_data *)(filp->private_data);

	switch (cmd) {
		case ENCDEC_CMD_CHANGE_KEY:
			private_data->key = arg;
			break;
		case ENCDEC_CMD_SET_READ_STATE:
			private_data->read_state = arg;
			break;
		case ENCDEC_CMD_ZERO:
			memset(device_buffer[minor], 0, memory_size);
			break;
	}

	return 0;
}

// Add implementations for:
// ------------------------

ssize_t encdec_read( struct file *filp, char *buf, size_t count, loff_t *f_pos, int minor) {
	if (*f_pos == memory_size) {
		return -EINVAL;
	}
	const size_t size_remaining = count < (memory_size - *f_pos) ? count : (memory_size - *f_pos);

	if (copy_to_user(buf, device_buffer[minor] + *f_pos, size_remaining) != 0) {
		return -EFAULT;
	}



	encdec_private_data * private_data = (encdec_private_data *)(filp->private_data);
	const unsigned char key = private_data->key;
	if (private_data->read_state == ENCDEC_READ_STATE_DECRYPT) {
		int i;
		for (i = 0; i < size_remaining; i++) {
			if (minor == 0) {
				buf[i] = (buf[i] + 128 - key) % 128;
			} else {
				buf[i] = buf[i] ^ key;
			}

		}
	}


	*f_pos += size_remaining;

	return size_remaining;
}

ssize_t encdec_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos, int minor) {
	if (*f_pos == memory_size) {
		return -ENOSPC;
	}
	const size_t size_remaining = count < (memory_size - *f_pos) ? count : (memory_size - *f_pos);

	char *s = kmalloc(size_remaining, GFP_KERNEL); // later try without allocating
    if (!s) {
        return -ENOMEM;
    }

	if (copy_from_user(s, buf, size_remaining) != 0) {
		kfree(s);
		return -EFAULT;
	}


	encdec_private_data * private_data = (encdec_private_data *)(filp->private_data);
	const unsigned char key = private_data->key;
	int i;
	for (i = 0; i < size_remaining; i++) {
		if (minor == 0) {
			s[i] = (s[i] + key) % 128;
		} else {
			s[i] = s[i] ^ key;
		}
	}

	strcat(device_buffer[minor], s);
	kfree(s);

	*f_pos += count;
	return count;
}

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos ) {
	return encdec_read(filp, buf, count, f_pos, 0);
}

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	return encdec_write(filp, buf, count, f_pos, 0);
}

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos ) {
	return encdec_read(filp, buf, count, f_pos, 1);
}

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	return encdec_write(filp, buf, count, f_pos, 1);
}
