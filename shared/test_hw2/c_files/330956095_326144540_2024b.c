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
MODULE_AUTHOR("shar-yashuv giat");

int 	encdec_open(struct inode *inode, struct file *filp);
int 	encdec_release(struct inode *inode, struct file *filp);
int 	encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);


int memory_size = 0;

MODULE_PARM(memory_size, "i");


char* caesarBuf, * xorBuf;
int major = 0;

const int caesar = 0;
const int xor = 1;

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


static ssize_t encdec_read_impl( struct file *filp, char* buf, size_t count, loff_t *f_pos, char (*decryptor)(loff_t *, encdec_private_date*));
static ssize_t encdec_write_impl(struct file *filp, const char* buf, size_t count, loff_t *f_pos, void (*coder)(char, loff_t *, encdec_private_date*));
static char decryptCaesar(loff_t *f_pos, encdec_private_date* data);
static char decryptXor(loff_t *f_pos, encdec_private_date* data);
static void encryptCaesar(char c, loff_t *f_pos, encdec_private_date* data);
static void encryptXor(char c, loff_t *f_pos, encdec_private_date* data);

#define KPRINT(...) printk(KERN_ALERT __VA_ARGS__)

int init_module(void)
{
	/*
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if(major < 0)
	{	
		return major;
	}

	if (!(caesarBuf = (char*)kmalloc(sizeof(char)* memory_size, GFP_KERNEL)) && !(xorBuf = (char*)kmalloc(sizeof(char) * memory_size, GFP_KERNEL))) {
		kfree(caesarBuf); // if caesarBuf alloc succeded we need to free it and if it failed it would be a no-op 
		//no need for the seacond one because it will always suceed
		KPRINT("kmalloc failed to allocate memory!\n");
		return (-1);
	}

	return 0;*/
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if(major < 0)
	{	
		return major;
	}

	//Allocate memory for the caesar device buffer
	caesarBuf = (char*)kmalloc(sizeof(char)* memory_size, GFP_KERNEL);
	if (caesarBuf == NULL) {
		printk(KERN_ALERT "kmalloc failed to allocate memory!\n");
		return (-1);
	}

	//Allocate memory for the xor device buffer
	xorBuf = (char*)kmalloc(sizeof(char) * memory_size, GFP_KERNEL);
	if (xorBuf == NULL) {
		printk(KERN_ALERT "kmalloc failed to allocate memory!\n");
		return (-1);
	}

	return 0;

}

void cleanup_module(void)
{
	unregister_chrdev(major, MODULE_NAME);	

	kfree(caesarBuf);
	kfree(xorBuf);
}

int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);
	//set 'filp->f_op' to the correct file - operations structure using the minor value to determine which
	filp->f_op = minor ? &fops_xor : &fops_caesar;
	if(((unsigned int)minor) > 1)
	{
		return -ENODEV;
	}

	//allocate memory for 'filp->private_data' as needed (using kmalloc)
	filp->private_data = (encdec_private_date*) kmalloc (sizeof(encdec_private_date), GFP_KERNEL);
	if (filp->private_data == NULL) {
		KPRINT("kmalloc failed to allocate memory!\n");
		return (-1);
	}

	((encdec_private_date*)filp->private_data)->key = 0;	
	((encdec_private_date*)filp->private_data)->read_state = ENCDEC_READ_STATE_DECRYPT;	

	return 0;

}

int encdec_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	const int minor = MINOR(inode->i_rdev);
	encdec_private_date* private_data = filp->private_data;;
	switch (cmd) {
		case ENCDEC_CMD_CHANGE_KEY:
		    private_data->key = arg & 0xFF;
		    break;
		case ENCDEC_CMD_SET_READ_STATE:
			private_data->read_state = arg & 0xFF;
		    break;
		case ENCDEC_CMD_ZERO:
			if(((unsigned int)minor)>1){
				return -ENODEV;
			}
			memset((minor ? xorBuf : caesarBuf), 0, memory_size);
			break;
	}
	return 0;
	

}



ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos ) {
	return encdec_read_impl(filp, buf, count, f_pos, decryptCaesar);
}

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos ) {
	return encdec_read_impl(filp, buf, count, f_pos, decryptXor);
}

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	return encdec_write_impl(filp, buf, count, f_pos, encryptCaesar);
}

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	return encdec_write_impl(filp, buf, count, f_pos, encryptXor);
}

static char decryptCaesar(loff_t *f_pos, encdec_private_date* data){
	char decoded = caesarBuf[(*f_pos)];
	if ((data->read_state) == ENCDEC_READ_STATE_DECRYPT) {			//We check if we want to decrypt the contents of the device's buffer
			decoded = ((decoded - (data->key)) + 128) % 128;		//deciphering	
	}
	return decoded;
}
static char decryptXor(loff_t *f_pos, encdec_private_date* data){
	char decoded = xorBuf[(*f_pos)];
	if ((data->read_state) == ENCDEC_READ_STATE_DECRYPT) {			//We check if we want to decrypt the contents of the device's buffer
			decoded ^= (data->key);		//deciphering	
	}
	return decoded;
}

static void encryptCaesar(char c, loff_t *f_pos, encdec_private_date* data){
	c = (c + (data->key)) % 128;		//Encryption
	caesarBuf[(*f_pos)] = c;

}
static void encryptXor(char c, loff_t *f_pos, encdec_private_date* data){
	c = c ^ (data->key);		//Encryption
	xorBuf[(*f_pos)] = c;

}

static ssize_t encdec_read_impl( struct file *filp, char* buf, size_t count, loff_t *f_pos, char (*decryptor)(loff_t *, encdec_private_date*)) {

	if ((*f_pos) >= memory_size)  // No further reading from the encryption device can be performed ,We exceeded the memory size
		return -EINVAL;

	char encodedValue;
	int numReadH = 0;
	encdec_private_date* data = filp->private_data;
	while (((*f_pos) < memory_size) && (numReadH < count))
	{
		encodedValue = decryptor(f_pos,data);
		copy_to_user((buf + numReadH), &encodedValue, sizeof (char));		//The transfer of information from the outside of the device to the outside of the user

		(*f_pos)++;
		numReadH++;
	}

	return numReadH;
}

static ssize_t encdec_write_impl(struct file *filp, const char* buf, size_t count, loff_t *f_pos, void (*coder)(char, loff_t *, encdec_private_date*)) {
	if ((*f_pos) >= memory_size)		// No further writing from the encryption device can be performed ,We exceeded the memory size
		return -ENOSPC;
	char encodedValue;
	int numWriteH = 0;
	encdec_private_date* data = filp->private_data;
	while (((*f_pos) < memory_size) && (numWriteH < count))
	{
		copy_from_user(&encodedValue, &(buf[numWriteH]), sizeof(char));
		coder(encodedValue,f_pos,data);

		(*f_pos)++;
		numWriteH++;
	}

	return numWriteH;		//The number of bytes actually write

}

#undef KPRINT

