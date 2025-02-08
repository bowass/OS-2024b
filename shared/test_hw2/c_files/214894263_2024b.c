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
MODULE_AUTHOR("ITAMAR HOEFLER");

int 	encdec_open(struct inode *inode, struct file *filp);
int 	encdec_release(struct inode *inode, struct file *filp);
int 	encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

int memory_size = 0;

MODULE_PARM(memory_size, "i");
char* caesar_buffer;
char* xor_buffer;
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
	caesar_buffer = kmalloc(sizeof(char) * memory_size,GFP_KERNEL);
	xor_buffer = kmalloc(sizeof(char) * memory_size,GFP_KERNEL);
	if (caesar_buffer == NULL || xor_buffer == NULL)
    {
        return -EFAULT;
    }
	return 0;
}

char caesar_encrypt(char c, int k)
{
    return ((c + k) % 128);
}

char caesar_decrypt(char c, int k)
{
    return ((c + 128 - k) % 128);
}

char xor(char c, int k)
{
    return (c ^ k);
}

void cleanup_module(void)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Unregister the device-driver
	// 2. Free the allocated device buffers using kfree
	unregister_chrdev(major, MODULE_NAME);
	kfree(caesar_buffer);
	kfree(xor_buffer);
}

int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);
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
    encdec_private_data* p;
    p = (encdec_private_data*) kmalloc(sizeof(encdec_private_data),GFP_KERNEL);
    if (p == NULL)
    {
        return -EFAULT;
    }
    p->key = 0;
    p->read_state = ENCDEC_READ_STATE_DECRYPT;
    filp->private_data = p;
	// Implemetation suggestion:
	// -------------------------
	// 1. Set 'filp->f_op' to the correct file-operations structure (use the minor value to determine which)
	// 2. Allocate memory for 'filp->private_data' as needed (using kmalloc)

	return 0;
}

int encdec_release(struct inode *inode, struct file *filp)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Free the allocated memory for 'filp->private_data' (using kfree)
	encdec_private_data* p = filp->private_data;
	kfree(p);
	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Update the relevant fields in 'filp->private_data' according to the values of 'cmd' and 'arg'
	int i;
	encdec_private_data* p = (encdec_private_data*) filp->private_data;
	switch(cmd)
	{
    case ENCDEC_CMD_CHANGE_KEY:
        {
            p->key = arg;
        }
        break;
    case ENCDEC_CMD_SET_READ_STATE:
        {
            if (arg == ENCDEC_READ_STATE_RAW)
            {
                p->read_state = ENCDEC_READ_STATE_RAW;
            }
            else if (arg == ENCDEC_READ_STATE_DECRYPT)
            {
                p->read_state = ENCDEC_READ_STATE_DECRYPT;
            }
        }
        break;
    case ENCDEC_CMD_ZERO:
        {
            int minor = MINOR(inode->i_rdev);
            if (minor == 0) //caesar
            {
                for (i = 0; i < memory_size; i++)
                {
                    caesar_buffer[i] = 0;
                }
            }
            else //xor
            {
                for (i = 0; i < memory_size; i++)
                {
                    xor_buffer[i] = 0;
                }
            }
        }
        break;
	}
	return 0;
}

// Add implementations for:
// ------------------------
// 1. ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
// 2. ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
// 3. ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
// 4. ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
ssize_t encdec_read_caesar(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
    int pass_count;
    int i;
    encdec_private_data* p = (encdec_private_data*) filp->private_data;
    if (*f_pos == memory_size)
    {
        return -EINVAL;
    }
    if (*f_pos + count > memory_size)
    {
        pass_count = memory_size - *f_pos;
    }
    else
    {
        pass_count = count;
    }
    if (copy_to_user(buf,caesar_buffer + sizeof(char) * (*f_pos),pass_count) != 0)
    {
        return -EFAULT;
    }
    *f_pos = *f_pos + pass_count;
    if (p->read_state == ENCDEC_READ_STATE_DECRYPT) //DECRYPT
    {
        for (i = 0; i < pass_count; i++)
        {
            buf[i] = caesar_decrypt(buf[i],p->key);
        }
    }
    return pass_count;
}

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos )
{
    int pass_count;
    int i;
    encdec_private_data* p = (encdec_private_data*) filp->private_data;
    if (*f_pos == memory_size)
        return -EINVAL;
    if (*f_pos + count > memory_size)
    {
        pass_count = memory_size - *f_pos;
    }
    else
    {
        pass_count = count;
    }
    if (copy_to_user(buf,xor_buffer + sizeof(char) * (*f_pos),pass_count) != 0)
    {
        return -EFAULT;
    }
    *f_pos = *f_pos + pass_count;
    if (p->read_state == ENCDEC_READ_STATE_DECRYPT) //DECRYPT
    {
        for (i = 0; i < pass_count; i++)
        {
            buf[i] = xor(buf[i],p->key);
        }
    }
    return pass_count;
}

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    int pass_count;
    int i;
    encdec_private_data* p = (encdec_private_data*) filp->private_data;
    if (*f_pos == memory_size)
        return -ENOSPC;
    if (*f_pos + count > memory_size)
    {
        pass_count = memory_size - *f_pos;
    }
    else
    {
        pass_count = count;
    }
    if (copy_from_user(caesar_buffer + sizeof(char) * (*f_pos),buf,pass_count) != 0)
    {
        return -EFAULT;
    }
    for (i = 0; i < pass_count; i++)
    {
        caesar_buffer[*f_pos] = caesar_encrypt(caesar_buffer[*f_pos],p->key);
        *f_pos += 1;
    }
    return pass_count;
}

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    int pass_count;
    int i;
    encdec_private_data* p = (encdec_private_data*) filp->private_data;
    if (*f_pos == memory_size)
        return -ENOSPC;
    if (*f_pos + count > memory_size)
    {
        pass_count = memory_size - *f_pos;
    }
    else
    {
        pass_count = count;
    }
    if (copy_from_user(xor_buffer + sizeof(char) * (*f_pos),buf,pass_count) != 0)
    {
        return -EFAULT;
    }
    for (i = 0; i < pass_count; i++)
    {
        xor_buffer[*f_pos] = xor(xor_buffer[*f_pos],p->key);
        *f_pos += 1;
    }
    return pass_count;
}
