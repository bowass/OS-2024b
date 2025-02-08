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
MODULE_AUTHOR("SHARON");

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

// Creating the needed buffers, for Caesar and XOR encryption.
char* caBuffer;
char* xorBuffer;

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


// The struct used to hold a read\write file's properties.
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

	// Allocating memory for the Caesar and XOR buffers.
	// If one of the allocates fails, the error ENOMEM (Error No Memory) will be returned.
	caBuffer = kmalloc(memory_size, GFP_KERNEL);
	if (!caBuffer)
	{
		return -ENOMEM;
	}

	xorBuffer = kmalloc(memory_size, GFP_KERNEL);
	if (!xorBuffer)
	{
		return -ENOMEM;
	}

	return 0;
}


void cleanup_module(void)
{
	unregister_chrdev(major, MODULE_NAME);
	
	// Freeing the memory used by the the Caesar and XOR buffers.
	kfree(caBuffer);
	kfree(xorBuffer);
}


int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);

	// Assigning the correct operation for Caesar / XOR encryption and decryption to a file,
	// based on the minor.
	// If the minor is not 0 or 1 (invalid minor), the error ENODEV is returned.
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

	// Allocating memory for the private data, to hold the file's properties.
	filp->private_data = (encdec_private_date*)kmalloc(sizeof(encdec_private_date), GFP_KERNEL); 

	// Creating a new encdec_private_date pointer, for accessing the file's properties.
	encdec_private_date* parameters = filp->private_data;

	// Initializing the file's properties as requested.
	parameters->key = 0;
	parameters->read_state = ENCDEC_READ_STATE_DECRYPT;
	
	return 0;
}


int encdec_release(struct inode *inode, struct file *filp)
{
	// The memory used to store the file's properties is released.
	kfree(filp->private_data);
	return 0;
}


int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	// Creating a new encdec_private_date pointer, for accessing the file's kept properties.
	encdec_private_date* parameters = filp->private_data;
	int minor = MINOR(inode->i_rdev);

	// Finding the requested command and operating based on it.
	switch (cmd)
	{
	case ENCDEC_CMD_CHANGE_KEY:
		// If the command calls to change the key, the file's new key will be the one given by the input, arg.
		parameters->key = arg;
		break;
		
	case ENCDEC_CMD_SET_READ_STATE:
		// If the command calls to change the reading state, the file's new read state will be the one given by the input, arg.
		if (arg == ENCDEC_READ_STATE_RAW)
		{
			parameters->read_state = ENCDEC_READ_STATE_RAW;
		}
		else if (arg == ENCDEC_READ_STATE_DECRYPT)
		{
			parameters->read_state = ENCDEC_READ_STATE_DECRYPT;
		}
		break;

	case ENCDEC_CMD_ZERO:
		// If the command calls to reset the file's buffer,
		// the Caesar or XOR will reset according to the minor.
		// If the minor is not 0 or 1 (invalid minor), the error ENODEV will be returned.
		if (minor == 0)
		{
			memset(caBuffer, 0, memory_size);
		}
		else if (minor == 1)
		{
			memset(xorBuffer, 0, memory_size);
		}
		else
		{
			return -ENODEV;
		}
		break;

	default:
		// If no legal command is found, the function returns the error ENOTTY.
		return -ENOTTY;
	}

	return 0;
}


ssize_t encdec_read_caesar(struct file* filp, char* buf, size_t count, loff_t* f_pos)
{
	// If there nothing left to read, the function will return the error EINVAL
	if (*f_pos == memory_size)
	{
		return -EINVAL;
	}

	// to_read is the amount of bytes that can and should be read.
	size_t to_read = min(count, memory_size - (size_t) *f_pos);

	// Creating a temporary buffer for keeping the read text and possibly decoding it.
	char* temp_buffer = kmalloc(to_read, GFP_KERNEL);
	if (!temp_buffer)
	{
		return -ENOMEM;
	}

	// Copying the read text from the buffer to the temporary buffer.
	memcpy(temp_buffer, caBuffer + *f_pos, to_read);

	// Creating a new encdec_private_date pointer for accessing the file's properties.
	encdec_private_date* parameters = filp->private_data;
	int rs = parameters->read_state;
	if (rs == ENCDEC_READ_STATE_DECRYPT)
	{
		unsigned char current_k = parameters->key;
		int i;
		for (i = 0; i < to_read; i++) // Decrypting the read text, if needed according to the file's properties.
		{
			temp_buffer[i] = (temp_buffer[i] + 128 - current_k) % 128;
		}
	}

	// Giving to the user the text (decoded or not)
	if (copy_to_user(buf, temp_buffer, to_read))
	{
		return -EFAULT;
	}

	// Freeing the memory used by the temporary buffer.
	kfree(temp_buffer);

	// Updating f_pos to point to the next place to read from the buffer (if it exists)
	*f_pos += to_read;

	// Returning the amount of bytes that were read.
	return to_read;
}


ssize_t encdec_write_caesar(struct file* filp, const char* buf, size_t count, loff_t* f_pos)
{
	// If there is no available space for writing in the buffer, the error ENOSPC will be returned.
	if (*f_pos == memory_size) {
		return -ENOSPC;
	}

	// to_write is the amount of bytes that can and need to be used for writing in the buffer.
	size_t to_write = min(count, memory_size - (size_t) *f_pos);

	// Creating a temporary buffer, for keeping the user's input and encrypting it.
	char* temp_buffer = kmalloc(to_write, GFP_KERNEL);
	if (!temp_buffer) {
		return -ENOMEM;
	}

	// Copying the user's input to the temporary buffer.
	if (copy_from_user(temp_buffer, buf, to_write)) {
		kfree(temp_buffer);
		return -EFAULT;
	}

	// Creating a new encdec_private_date pointer to access the file's properties.
	encdec_private_date* parameters = filp->private_data;
	unsigned char current_k = parameters->key;
	int i;
	for (i = 0; i < to_write; i++) // Encoding the given text.
	{
		temp_buffer[i] = (temp_buffer[i] + current_k) % 128;
	}
	
	// Keeping the encrypted text in the buffer.
	memcpy(caBuffer + *f_pos, temp_buffer, to_write);
	
	// Freeing the memory used in the temporary buffer.
	kfree(temp_buffer);

	// Updating f_pos to the first place in the buffer that has free bytes (if it exists)
	*f_pos += to_write;

	// Returning the amount of bytes that were filled with new text in the buffer.
	return to_write;
}



ssize_t encdec_read_xor(struct file* filp, char* buf, size_t count, loff_t* f_pos)
{
	// If there nothing left to read, the function will return the error EINVAL.
	if (*f_pos == memory_size)
	{
		return -EINVAL;
	}

	// to_read is the amount of bytes that can and should be read.
	size_t to_read = min(count, memory_size - (size_t)*f_pos);

	// Creating a temporary buffer for keeping the read text and possibly decoding it.
	char* temp_buffer = kmalloc(to_read, GFP_KERNEL);
	if (!temp_buffer)
	{
		return -ENOMEM;
	}
	
	// Copying the read text from the buffer to the temporary buffer.
	memcpy(temp_buffer, xorBuffer + *f_pos, to_read);

	// Creating a new encdec_private_date pointer for accessing the file's properties.
	encdec_private_date* parameters = filp->private_data;
	int rs = parameters->read_state;

	if (rs == ENCDEC_READ_STATE_DECRYPT) // Decrypting the read text, if needed according to the file's properties.
	{
		unsigned char current_k = parameters->key;
		int i;
		for (i = 0; i < to_read; i++)
		{
			temp_buffer[i] = temp_buffer[i] ^ current_k;
		}
	}
	
	// Giving to the user the text (decoded or not)
	if (copy_to_user(buf, temp_buffer, to_read))
	{
		return -EFAULT;
	}

	// Freeing the memory used by the temporary buffer.
	kfree(temp_buffer);

	// Updating f_pos to point to the next place to read from the buffer (if it exists)
	*f_pos += to_read;

	// Returning the amount of bytes that were read.
	return to_read;
}


ssize_t encdec_write_xor(struct file* filp, const char* buf, size_t count, loff_t* f_pos)
{
	// If there no space for writing, the function will return the error EINVAL
	if (*f_pos == memory_size)
	{
		return -ENOSPC;
	}

	// to_write is the amount of bytes that can and need to be used for writing in the buffer.
	size_t to_write = min(count, memory_size - (size_t) *f_pos);

	// Allocating space for a temporary buffer, used for keeping the input and encrypting it.
	char* temp_buffer = kmalloc(to_write, GFP_KERNEL);
	if (!temp_buffer)
	{
		return -ENOMEM;
	}

	// Copying the user's input to the temporary buffer.
	if (copy_from_user(temp_buffer, buf, to_write))
	{
		return -EFAULT;
	}

	// Creating a new encdec_private_date pointer to access the file's properties.
	encdec_private_date* parameters = filp->private_data;
	unsigned char current_k = parameters->key;

	int i;
	for (i = 0; i < to_write; i++) // Encoding the given text.
	{
		temp_buffer[i] = temp_buffer[i] ^ current_k;
	}

	// Keeping the encrypted text in the buffer.
	memcpy(xorBuffer + *f_pos, temp_buffer, to_write);
	
	// Freeing the memory used in the temporary buffer.
	kfree(temp_buffer);

	// Updating f_pos to the first place in the buffer that has free bytes (if it exists)
	*f_pos += to_write;

	// Returning the amount of bytes that were filled with new text in the buffer.
	return to_write;
}
