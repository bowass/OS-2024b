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


char* buffer_caesar;
char* buffer_xor;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YOUR NAME");

void set_encryption_key(struct file *flip, const int key);
void set_read_state(struct file *flip,const int read_state);

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
// -------------------------
// Use this structure as your file-object's private data structure
typedef struct {
	unsigned char key;
	int read_state;
} encdec_private_date;

void set_encryption_key(struct file *flip,const int key){
	encdec_private_date* x=(encdec_private_date*)flip->private_data;
	x->key=key;
}

void set_read_state(struct file *flip,const int read_state){
	encdec_private_date* x=(encdec_private_date*)flip->private_data;
	x->read_state=read_state; 
}

int init_module(void)
{
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if(major < 0)
	{	
		return major;
	}
	buffer_caesar = (char*)kmalloc(memory_size, GFP_KERNEL);
	buffer_xor = (char*)kmalloc(memory_size, GFP_KERNEL);

	//maybe add somthing that check mistakes,like:
	if(!buffer_caesar||!buffer_xor){
	return -ENOMEM; 
	}
	return 0;
}

void cleanup_module(void)
{
	unregister_chrdev(major, MODULE_NAME);
	kfree(buffer_caesar);
	kfree(buffer_xor);
}

int encdec_open(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);
	if (minor == 0) {
		filp->f_op = &fops_caesar;
	}
	else if (minor == 1) {
		filp->f_op = &fops_xor;
	}
	else {
		return -ENODEV;
	}
	filp->private_data =  kmalloc(sizeof(encdec_private_date),GFP_KERNEL);
	encdec_private_date* x=(encdec_private_date*)filp->private_data;
	
	/*should i had a defult values? like:*/
	if (x == NULL) {
    // Handle allocation failure
    	return -ENOMEM; // or another appropriate error code
	}
	
	x->key=0;
	x->read_state=ENCDEC_READ_STATE_DECRYPT;
	return 0;
}



int encdec_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int minor=-1;
	encdec_private_date* x=(encdec_private_date*)filp->private_data;
	switch (cmd) {/*maybe do somthing before switch, do encdec_private_date tmp=flipprivate_date; but i dont think so*/
		case ENCDEC_CMD_CHANGE_KEY:
			x->key = arg;
			break;
		case ENCDEC_CMD_SET_READ_STATE:
			x->read_state = arg;/*not sure what to put in... */
			break;
		case ENCDEC_CMD_ZERO:
			minor= MINOR(inode->i_rdev);
			if (minor == 0) {
				memset(buffer_caesar, 0, memory_size);
			}
			else if (minor == 1) {
				memset(buffer_xor, 0, memory_size);
			}
			else {
				return -ENODEV;
			}
			break;
	}

	return 0;
}


//https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch03s08.html
ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos ) {
	encdec_private_date* x=(encdec_private_date*)filp->private_data;
	size_t num=min(count,memory_size-(size_t)*f_pos);
	char *temp=kmalloc(sizeof(char)*num,GFP_KERNEL);
	if (*f_pos == memory_size) {
		return -EINVAL;
	}
	
	if (x->read_state == ENCDEC_READ_STATE_DECRYPT) {
		unsigned char k = x->key;
		int i;
		for (i = 0; i < num ; i++) {
			temp[i] = (buffer_caesar[i+*f_pos] + 128 - k) % 128;
		}
	}
	else{
		int i=0;
		for ( i = 0; i < num ; i++) {
			temp[i] = (buffer_caesar[i+*f_pos]) ;
		}
	}
	if(copy_to_user(buf,temp,num)){
		return -EFAULT;
	}
	kfree(temp);
	printk("%s",buffer_caesar);
	*f_pos = *f_pos + num;/*לבדוק אם צריך לטפל במקרה שלא הוצלחתי להעתיך את הכל כלמור יש לי מקום להעתיק רק חלק*/
	return num;
}

ssize_t encdec_write_caesar(struct file* filp, const char* buf, size_t count, loff_t* f_pos) {
	encdec_private_date* x=(encdec_private_date*)filp->private_data;
	if (*f_pos == memory_size) {
		return -ENOSPC;
	}
	size_t num=min(count,memory_size-(size_t)*f_pos);
	if (copy_from_user(buffer_caesar + *f_pos, buf, num)) {
		return -EFAULT;
	}
	unsigned char k =x->key;
	int i;
	for (i = *f_pos; i < num+ *f_pos ; i++)
	{
		buffer_caesar[i] = (buffer_caesar[i] + k) % 128;
	}
	*f_pos = *f_pos + num;
	return num;
}

ssize_t encdec_read_xor(struct file* filp, char* buf, size_t count, loff_t* f_pos) {
	encdec_private_date* x=(encdec_private_date*)filp->private_data;
	size_t num=min(count,memory_size-(size_t)*f_pos);
	char *temp=kmalloc(sizeof(char)*num,GFP_KERNEL);
	if (*f_pos == memory_size) {
		return -EINVAL;
	}
	if (x->read_state == ENCDEC_READ_STATE_DECRYPT) {
		unsigned char k = x->key;
		int i;
		for (i = 0; i < num ; i++) {
			temp[i] = (buffer_xor[i+*f_pos]^k) ;
		}
	}
	else{
		int i=0;
		for ( i = 0; i < num ; i++) {
			temp[i] = (buffer_xor[i+*f_pos] ) ;
		}
	}
	if(copy_to_user(buf,temp,num)){
		return -EFAULT;
	}
	kfree(temp);
	
	printk("read xor-%s\n",buffer_xor);
	return num;
}

ssize_t encdec_write_xor(struct file* filp, const char* buf, size_t count, loff_t* f_pos) {
	encdec_private_date* x=(encdec_private_date*)filp->private_data;
	size_t num=min(count,memory_size-(size_t)*f_pos);
	if (*f_pos == memory_size) {
		return -ENOSPC;
	}
	if (copy_from_user(buffer_xor + *f_pos, buf, num)) {
		return -EFAULT;
	}
	unsigned char k =x->key;
	int i;
	for (i = *f_pos; i < num+ *f_pos ; i++)
	{
		buffer_xor[i] = (buffer_xor[i] ^ k);
	}
	*f_pos = *f_pos + num;
	return num;
}

