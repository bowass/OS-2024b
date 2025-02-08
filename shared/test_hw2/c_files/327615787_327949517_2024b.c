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
// ------------------------
// Use this structure as your file-object's private data structure
typedef struct {
	unsigned char key;
	int read_state;
} encdec_private_data; // data not date


char* data_buffer0;
char* data_buffer1;








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

	data_buffer0 = (char*)kmalloc(memory_size, GFP_KERNEL); // do the memory_size*sizeof(char*) or just memory_size ?
	data_buffer1 = (char*)kmalloc(memory_size, GFP_KERNEL);
	if(data_buffer0 == NULL || data_buffer1 == NULL){
        return -1;
	}


	return 0;
}

void cleanup_module(void)
{
	// Implemetation suggestion:
	// -------------------------
	// 1. Unregister the device-driver
	// 2. Free the allocated device buffers using kfree

	if(unregister_chrdev(major, MODULE_NAME) < 0){
        // error
	}

	kfree(data_buffer0);
	kfree(data_buffer1);


}

int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);

	// Implemetation suggestion:
	// -------------------------
	// 1. Set 'filp->f_op' to the correct file-operations structure (use the minor value to determine which)
	// 2. Allocate memory for 'filp->private_data' as needed (using kmalloc)
	if(minor == 0){
        filp->f_op = &fops_caesar;
	}
	else if(minor == 1){
        filp->f_op = &fops_xor;
	}
	else{
        // error
        return -ENODEV;
	}

	encdec_private_data* epd = NULL;
	epd = kmalloc(sizeof(encdec_private_data), GFP_KERNEL);
	if(epd == NULL){

        return -1;
	}
	epd->key = 0;
	epd->read_state = ENCDEC_READ_STATE_DECRYPT;

	filp->private_data = epd;



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

	encdec_private_data* epd; // cannot use filp->private_data->key because the compilre thinks its a void pointer we cannot access

	//if(filp->private_data == NULL){
        //error
	//}

	epd = filp->private_data;

	switch(cmd){
        case ENCDEC_CMD_CHANGE_KEY:
            epd->key = arg; // is it just the arg ?
            break;
        case ENCDEC_CMD_SET_READ_STATE:
            epd->read_state = arg;
            break;
        case ENCDEC_CMD_ZERO:
            // zero the buffer. maybe with the /dev/zero from the tirgul or what they say in the pdf
            // or memset
            // check to what buffer do the zero by the minor
            switch(minor){
                case 0:
                    memset(data_buffer0, 0, memory_size); // maybe another way to zero?
                    break;
                case 1:
                    memset(data_buffer1, 0, memory_size);
                    break;
                default:
                    return -ENODEV;
                    break;
            }

            break;
	}




	return 0;
}

// Add implementations for:
// ------------------------
ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos ){
    if(*f_pos == memory_size)
        return -EINVAL;

    encdec_private_data* epd;
    epd = filp->private_data;

    char k = epd->key;

    int len_read = 0;

    if(count <= memory_size - *f_pos){
        len_read = count;
    }
    else{
        len_read = memory_size - *f_pos;
    }


    char* temp = (char*)kmalloc(len_read, GFP_KERNEL);

    if(temp == NULL){
        return -1;
    }

    int i = 0;

    if(epd->read_state == ENCDEC_READ_STATE_DECRYPT){
        for(i = 0; i<len_read; i++){
            temp[i] = (data_buffer0[i + *f_pos] + 128 - k) % 128;
        }
    }
    else{
        for(i = 0; i<len_read; i++){
            temp[i] = data_buffer0[i + *f_pos];
        }
    }

    int bytes = copy_to_user(buf, temp, len_read);

    if(bytes != 0){
        return -EFAULT;
    }

    *f_pos += len_read;

    kfree(temp);


    return len_read;
}


ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos){
    if(*f_pos == memory_size)
        return -ENOSPC;

    encdec_private_data* epd;
    epd = filp->private_data;

    char k = epd->key;
    int len_write = 0; // initialize?

    if(count <= memory_size - *f_pos){
        len_write = count;
    }
    else{
        len_write = memory_size - *f_pos;
    }

    int bytes = copy_from_user(data_buffer0 + *f_pos, buf, len_write);

    if(bytes != 0){
        return -EFAULT;
    }

    int i;

    for(i = *f_pos; i<len_write + *f_pos; i++){
        data_buffer0[i] = (data_buffer0[i] + k) % 128;
    }

    *f_pos += len_write;

    return len_write;

}



ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos ){
    if(*f_pos == memory_size)
        return -EINVAL;

    encdec_private_data* epd;
    epd = filp->private_data;

    char k = epd->key;

    int len_read = 0;

    if(count <= memory_size - *f_pos){
        len_read = count;
    }
    else{
        len_read = memory_size - *f_pos;
    }


    char* temp = (char*)kmalloc(len_read, GFP_KERNEL);

    if(temp == NULL){
        return -1;
    }

    int i = 0;

    if(epd->read_state == ENCDEC_READ_STATE_DECRYPT){
        for(i = 0; i<len_read; i++){
            temp[i] = data_buffer1[i + *f_pos] ^ k;
        }
    }
    else{
        for(i = 0; i<len_read; i++){
            temp[i] = data_buffer1[i + *f_pos];
        }
    }

    int bytes = copy_to_user(buf, temp, len_read);

    if(bytes != 0){
        return -EFAULT;
    }

    *f_pos += len_read;

    kfree(temp);


    return len_read;
}



ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos){
    if(*f_pos == memory_size)
        return -ENOSPC;

    encdec_private_data* epd;
    epd = filp->private_data;

    char k = epd->key;
    int len_write = 0; // initialize?

    if(count <= memory_size - *f_pos){
        len_write = count;
    }
    else{
        len_write = memory_size - *f_pos;
    }

    int bytes = copy_from_user(data_buffer1 + *f_pos, buf, len_write);

    if(bytes != 0){
        return -EFAULT;
    }

    int i;

    for(i = *f_pos; i<len_write + *f_pos; i++){
        data_buffer1[i] = data_buffer1[i] ^ k;
    }

    *f_pos += len_write;

    return len_write;
}









