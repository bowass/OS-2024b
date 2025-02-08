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
#define DEFAULT_KEY 0

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
char * data_buffer_caesar;
char * data_buffer_xor;

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
} encdec_private_date;

void zero_buffer ( char* buffer)
{
    int i;
    for(i = 0; i < memory_size; i++ )
    {
        buffer[i] = 0;
    }
}




int init_module(void)
{

	major = register_chrdev(major, MODULE_NAME, &fops_caesar);

	if(major < 0)
	{
		return major;
	}

	data_buffer_caesar = (char*)kmalloc(memory_size, GFP_KERNEL);
    data_buffer_xor = (char*)kmalloc(memory_size, GFP_KERNEL);
    zero_buffer(data_buffer_caesar);
    zero_buffer(data_buffer_xor);
	return 0;
}

void cleanup_module(void)
{
    //Unregister the device-driver
    unregister_chrdev(major, MODULE_NAME);
    //Free the allocated device buffers using kfree
    kfree(data_buffer_caesar);
    kfree(data_buffer_xor);
}

int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);
	//choose file operations structure by minor
	if(minor == XOR_MINOR)
    {
        filp->f_op = &fops_xor;
    }
    else if(minor != CAESAR_MINOR) //minor is not valid
    {
        return -ENODEV;
    }

    //Allocate memory for private data
    filp->private_data = (encdec_private_date* )kmalloc(sizeof(encdec_private_date), GFP_KERNEL);
    //init those fields
    encdec_private_date* temp = (encdec_private_date*) filp->private_data;
    temp->key = DEFAULT_KEY;
    temp->read_state = ENCDEC_READ_STATE_DECRYPT;
	return 0;
}

int encdec_release(struct inode *inode, struct file *filp)
{
    //Free the allocated memory for 'filp->private_data'
    kfree(filp->private_data);
	return 0;
}

//איך זה עובד? איפה מוגדרות הפונקצות המיוחדות ומי נותן להן את המספרים שלהן?????????

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    encdec_private_date* temp = (encdec_private_date*) filp->private_data;
    switch( cmd )
    {
        case ENCDEC_CMD_CHANGE_KEY :
            temp->key = arg;
            break;
        case ENCDEC_CMD_SET_READ_STATE :
            temp->read_state = arg;
            break;
        case ENCDEC_CMD_ZERO :
            if(MINOR(inode->i_rdev) == CAESAR_MINOR)
            {
                zero_buffer(data_buffer_caesar);
            }
            else if (MINOR(inode->i_rdev) == XOR_MINOR)
            {
                zero_buffer(data_buffer_xor);
            }
            else
            {
                return -ENODEV;
            }
            break;

    }

	return 0;
}

void caesar_encode(loff_t start_index, size_t size, int key)
{
    int i;
    for (i = 0; i < size; i++)
    {
        data_buffer_caesar[start_index + i] = (data_buffer_caesar[start_index + i] + key) % 128;
    }
}

void xor_encode(loff_t start_index, size_t size, int key)
{
    int i;
    for (i = 0; i < size; i++)
    {
        data_buffer_xor[start_index + i] = (data_buffer_xor[start_index + i] ^ key);
    }
}

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    encdec_private_date* temp = (encdec_private_date*) filp->private_data;
    int available_space = memory_size - (*f_pos);
    if (available_space == 0)
    {
        return -ENOSPC;
    }
    int minimum_to_write = (available_space <= count) ? available_space : count;
    if (copy_from_user(&data_buffer_caesar[*f_pos], buf, minimum_to_write) > 0)
        return -EFAULT;
    caesar_encode(*f_pos, minimum_to_write, temp->key);
    *f_pos += minimum_to_write;
    return minimum_to_write;
}

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    encdec_private_date* temp = (encdec_private_date*) filp->private_data;
    int available_space = memory_size - (*f_pos);
    if (available_space == 0)
    {
        return -ENOSPC;
    }
    int minimum_to_write = (available_space <= count) ? available_space : count;
    if (copy_from_user(&data_buffer_xor[*f_pos], buf, minimum_to_write) > 0)
        return -EFAULT;
    xor_encode(*f_pos, minimum_to_write, temp->key);
    *f_pos += minimum_to_write;
    return minimum_to_write;
}


void caesar_decode(char* user_buf, size_t size, int key)
{
    int i;
    for (i = 0; i< size; i++)
    {
        user_buf[i] = (user_buf[i] + 128 - key) % 128;
    }
}

void xor_decode(char* user_buf, size_t size, int key)
{
    int i;
    for (i = 0; i< size; i++)
    {
        user_buf[i] = (user_buf[i] ^ key);
    }
}


ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos )
{
    encdec_private_date* temp = (encdec_private_date*) filp->private_data;
    int available_space = memory_size - (*f_pos);

    if( available_space == 0)
    {
        return -EINVAL;
    }
    int minimum_to_read = (available_space <= count) ? available_space : count;
    if(copy_to_user (buf, &data_buffer_caesar[*f_pos], minimum_to_read) > 0)
        return -EFAULT;
    if (temp->read_state == ENCDEC_READ_STATE_DECRYPT)
    {
        caesar_decode(buf, minimum_to_read, temp->key);
    }
    *f_pos += minimum_to_read;
    return minimum_to_read;

}


ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos )
{
    encdec_private_date* temp = (encdec_private_date*) filp->private_data;
    int available_space = memory_size - (*f_pos);

    if( available_space == 0)
    {
        return -EINVAL;
    }
    int minimum_to_read = (available_space <= count) ? available_space : count;
    if(copy_to_user (buf, &data_buffer_xor[*f_pos], minimum_to_read) > 0)
        return -EFAULT;
    if (temp->read_state == ENCDEC_READ_STATE_DECRYPT)
    {
        xor_decode(buf, minimum_to_read, temp->key);
    }
    *f_pos += minimum_to_read;
    return minimum_to_read;
}

