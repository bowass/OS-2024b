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
MODULE_AUTHOR("Ronnie Rasamat & Tomer Yarkoni");
//submitters: Ronnie Rasamat 216298596, Tomer Yarkoni 327623781
typedef enum {
	Caesar,
	Xor,
} cipher_t;

int 	encdec_open(struct inode *inode, struct file *filp);
int 	encdec_release(struct inode *inode, struct file *filp);
int 	encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

static ssize_t encdec_read( struct file *filp, char *buf, size_t count, loff_t *f_pos, const cipher_t cipher);
static ssize_t encdec_write( struct file *filp, const char *buf, size_t count, loff_t *f_pos, const cipher_t cipher);

int memory_size = 0;

MODULE_PARM(memory_size, "i");

int major = 0;

struct file_operations fops[2] = {{
	.open 	 =	encdec_open,
	.release =	encdec_release,
	.read 	 =	encdec_read_caesar,
	.write 	 =	encdec_write_caesar,
	.llseek  =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
}, {
	.open 	 =	encdec_open,
	.release =	encdec_release,
	.read 	 =	encdec_read_xor,
	.write 	 =	encdec_write_xor,
	.llseek  =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
}};

typedef struct {
	u8 key;
	u8 read_state;
} encdec_private_date;

static char* buffers[2];

int init_module(void) {
	major = register_chrdev(major, MODULE_NAME, &fops[Caesar]);
	if(major < 0) return major;

	buffers[Caesar] = kmalloc(memory_size, GFP_KERNEL);
	buffers[Xor] = kmalloc(memory_size, GFP_KERNEL);
	return 0;
}

void cleanup_module(void) {
	unregister_chrdev(major, MODULE_NAME);
	kfree(buffers[Caesar]);
	kfree(buffers[Xor]);
}

int encdec_open(struct inode *inode, struct file *filp) {
	const int minor = MINOR(inode->i_rdev);
	switch (minor) {
		case Caesar:
		case Xor:
			filp->f_op = &fops[minor];
			break;
		default:
			return -ENODEV;
	}
	encdec_private_date* const private_data = kmalloc(sizeof(encdec_private_date), GFP_KERNEL);
	private_data->key = 0;
	private_data->read_state = ENCDEC_READ_STATE_DECRYPT;

    filp->private_data = private_data;
	return 0;
}

int encdec_release(struct inode *inode, struct file *filp) {
	kfree(filp->private_data);
	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
	const int minor = MINOR(inode->i_rdev);
	encdec_private_date* const private_data = filp->private_data;
	switch (cmd) {
		case ENCDEC_CMD_CHANGE_KEY:
		    private_data->key = arg & 0xFF;
		    break;
		case ENCDEC_CMD_SET_READ_STATE:
			private_data->read_state = arg & 0xFF;
		    break;
		case ENCDEC_CMD_ZERO:
			switch (minor) {
				case Caesar:
				case Xor:
                    memset(buffers[minor], 0, memory_size);
                    break;
                default:
                    return -ENODEV;
			}
			break;
	}
	return 0;
}

ssize_t encdec_read_caesar(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
	return encdec_read(filp, buf, count, f_pos, Caesar);
}

ssize_t encdec_read_xor(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
	return encdec_read(filp, buf, count, f_pos, Xor);
}

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	return encdec_write(filp, buf, count, f_pos, Caesar);
}

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	return encdec_write(filp, buf, count, f_pos, Xor);
}

static ssize_t encdec_write(struct file *filp, const char* buf, size_t count, loff_t *f_pos, const cipher_t cipher) {
	const encdec_private_date* const private_data = filp->private_data;
	const u8 key = private_data->key;
	const loff_t pos = *f_pos;
	const size_t free_bytes = memory_size - pos;
	
	size_t bytes_to_copy = count < free_bytes ? count : free_bytes;
	if (!bytes_to_copy) return -ENOSPC;

	char* dst = buffers[cipher] + pos;
	const size_t bytes_not_copied = copy_from_user(dst, buf, bytes_to_copy);
	if (bytes_not_copied) return -EFAULT;

	*f_pos += bytes_to_copy;

	if (cipher == Caesar) do { *dst += key; *dst++ &= 0x7F; } while (--bytes_to_copy);
	else				  do { *dst++ ^= key; } while (--bytes_to_copy);
	
	return *f_pos - pos;
}

static ssize_t encdec_read(struct file *filp, char* buf, size_t count, loff_t *f_pos, const cipher_t cipher) {
	const encdec_private_date* const private_data = filp->private_data;
	const u8 key = private_data->key;
	const loff_t pos = *f_pos;
	const size_t free_bytes = memory_size - pos;

	size_t bytes_to_copy = count < free_bytes ? count : free_bytes;
	if (!bytes_to_copy) return -EINVAL;

	const char* src = buffers[cipher] + pos;
	const size_t bytes_not_copied = copy_to_user(buf, src, bytes_to_copy);
	if (bytes_not_copied) return -EFAULT;
    
	*f_pos += bytes_to_copy;

	switch (private_data->read_state) {
		case ENCDEC_READ_STATE_RAW: return bytes_to_copy;
        case ENCDEC_READ_STATE_DECRYPT: if (cipher == Caesar) do { *buf += 128 - key; *buf++ &= 0x7F; } while (--bytes_to_copy);
										else 				  do { *buf++ ^= key; } while (--bytes_to_copy); 
	}
	return *f_pos - pos;
}
