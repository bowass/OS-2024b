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
#define CAESAR 0
#define XOR 1

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Elad & Inbal");

int 	encdec_open(struct inode *inode, struct file *filp);
int 	encdec_release(struct inode *inode, struct file *filp);
int 	encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

/*Assistance functions*/
ssize_t rawRead(struct file* filp, char* buf, size_t count, loff_t* f_pos, int xor_or_caesar);
ssize_t decryptRead(struct file* filp, char* buf, size_t count, loff_t* f_pos, int xor_or_caesar);
ssize_t endec_write(struct file* filp, const char* buf, size_t count, loff_t* f_pos, int xor_or_caesar);
ssize_t endec_read(struct file* filp, char* buf, size_t count, loff_t* f_pos, int xor_or_caesar);

int memory_size = 0;


MODULE_PARM(memory_size, "i");

int major = 0;

void* arr[2];

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

int init_module(void)
{
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if(major < 0)
	{	
		return major;
	}
	arr[CAESAR] = kmalloc(memory_size, GFP_KERNEL);
	arr[XOR] = kmalloc(memory_size, GFP_KERNEL);
	return 0;
}

void cleanup_module(void)
{
	unregister_chrdev(major, MODULE_NAME);
	kfree(arr[CAESAR]);
	kfree(arr[XOR]);
}

int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);
	size_t pdSize = 5;
	filp->f_op = ((minor) ? (&fops_xor) : (&fops_caesar));
	void* myData = kmalloc(pdSize, GFP_KERNEL);
	if (!myData) return -ENOMEM; /*Allocation failed*/
	filp->private_data = myData;
	((encdec_private_date*)myData)->read_state = ENCDEC_READ_STATE_DECRYPT;
	((encdec_private_date*)myData)->key = 0;
	return 0;
}

int encdec_release(struct inode *inode, struct file *filp)
{
	void* myData = filp->private_data;
	kfree(myData);
	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	encdec_private_date* myData = (encdec_private_date*)filp->private_data;
	if (cmd == ENCDEC_CMD_CHANGE_KEY) {
		myData->key = arg;
	}
	else if (cmd == ENCDEC_CMD_SET_READ_STATE)
	{
		myData->read_state = arg;
	}
	else { /*cmd == ENCDEC_CMD_ZERO*/
		int i;
		int minor = MINOR(inode->i_rdev);
		char* buffer = (char*)(minor ? arr[XOR] : arr[CAESAR]);
		for (i = 0; i < memory_size; i++) {
			buffer[i] = '\0';
		}
	}
	return 0;
}

ssize_t encdec_read_caesar(struct file* filp, char* buf, size_t count, loff_t* f_pos) {
	return endec_read(filp, buf, count, f_pos, CAESAR);
}	

ssize_t encdec_read_xor(struct file* filp, char* buf, size_t count, loff_t* f_pos) {
	return endec_read(filp, buf, count, f_pos, XOR);
}

ssize_t endec_read(struct file* filp, char* buf, size_t count, loff_t* f_pos, int xor_or_caesar) {
	if (!count) return 0;
	if (((encdec_private_date*)(filp->private_data))->read_state == ENCDEC_READ_STATE_RAW) {
		/*return raw data*/
		return rawRead(filp, buf, count, f_pos, xor_or_caesar);
	}
	else {
		/*return decrypted data*/
		return decryptRead(filp, buf, count, f_pos, xor_or_caesar);
	}
}

ssize_t rawRead(struct file* filp, char* buf, size_t count, loff_t* f_pos, int xor_or_caesar) {
	int failSize;
	if (*f_pos >= (loff_t)memory_size) {
		return -EINVAL; /*Nothing was read - reached EOF*/
	}
	else {
		int position = (int)(*f_pos);
		int copySize = (count < memory_size - position) ? (count) : (memory_size - position);
		failSize = copy_to_user(buf, arr[xor_or_caesar] + *f_pos, copySize);
		int readSize = count - failSize;
		if (readSize != copySize) return -EFAULT; /*Error in copy_to_user*/
		/*Something was read. return number of bytes read*/
		*f_pos += (loff_t)readSize;
		return readSize;		
	}
}

ssize_t decryptRead(struct file* filp, char* buf, size_t count, loff_t* f_pos, int xor_or_caesar) {
	int i;
	int position = *f_pos;
	char* C = (char*)arr[xor_or_caesar];
	char* dData = (char*)kmalloc(count, GFP_KERNEL);
	char key = ((encdec_private_date*)(filp->private_data))->key;
	for (i = 0; i < count && (*f_pos + i) < memory_size; i++) { /*Check memory size*/
		if (xor_or_caesar) { /*XOR*/
			dData[i] = C[position + i];
			dData[i] = dData[i] ^ key;
		}
		else { /*CAESAR*/
			dData[i] = C[position + i]; 
			dData[i] = (dData[i] + 128 - key) % 128;
		}
	}
	int failSize = copy_to_user(buf, dData, i);
	if (failSize != 0) return -EFAULT; /*Error in copy_to_user*/
	*f_pos += (loff_t)i; /*Update read place*/
	kfree(dData); /*Free allocated memory*/
	if (i == 0) return -EINVAL; /*Nothing was read*/
	else return i; /*Number of bytes copied*/
}

ssize_t encdec_write_caesar(struct file* filp, const char* buf, size_t count, loff_t* f_pos) {
	return endec_write(filp, buf, count, f_pos, CAESAR);
}

ssize_t encdec_write_xor(struct file* filp, const char* buf, size_t count, loff_t* f_pos) {
	return endec_write(filp, buf, count, f_pos, XOR);
}

ssize_t endec_write(struct file* filp, const char* buf, size_t count, loff_t* f_pos, int xor_or_caesar) {
	if (*f_pos >= (loff_t)memory_size) { /*Buffer full*/
		return -ENOSPC;
	}
	else { 
		int position = (int)(*f_pos);
		int copySize = (count < memory_size - position) ? (count) : (memory_size - position);
		char* C = (char*)arr[xor_or_caesar];
		int	failSize = copy_from_user(C + position, buf, copySize);
		int succesSize = count - failSize;
		if (succesSize != copySize) return -EFAULT; /*Error in copy_from_user*/
		int i;
		int key = ((encdec_private_date*)(filp->private_data))->key;
		for (i = 0; i < succesSize; i++) {
			if (xor_or_caesar) { /*XOR*/
				C[i + position] = C[i + position] ^ key;
			}
			else { /*CAESAR*/
				C[i + position] = (C[i + position] + key) % 128;
			}
		}
		*f_pos += (loff_t)succesSize; /*Update *f_pos value*/
		return (ssize_t)succesSize;
	}
}


