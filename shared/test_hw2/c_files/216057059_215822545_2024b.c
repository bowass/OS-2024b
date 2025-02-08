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

int     encdec_open(struct inode *inode, struct file *filp);
int     encdec_release(struct inode *inode, struct file *filp);
int     encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

int memory_size = 0;

MODULE_PARM(memory_size, "i");

int major = 0;

struct file_operations fops_caesar = {
    .open      =    encdec_open,
    .release =    encdec_release,
    .read      =    encdec_read_caesar,
    .write      =    encdec_write_caesar,
    .llseek  =    NULL,
    .ioctl      =    encdec_ioctl,
    .owner      =    THIS_MODULE
};

struct file_operations fops_xor = {
    .open      =    encdec_open,
    .release =    encdec_release,
    .read      =    encdec_read_xor,
    .write      =    encdec_write_xor,
    .llseek  =    NULL,
    .ioctl      =    encdec_ioctl,
    .owner      =    THIS_MODULE
};


typedef struct {
    unsigned char key;
    int read_state;
} encdec_private_data;


char* caesarBuffer = NULL;
int caesarBufferLen = 0;
char* xorBuffer = NULL;
int xorBufferLen = 0;

int init_module(void)
{
    major = register_chrdev(major, MODULE_NAME, &fops_caesar);
    if(major < 0)
    {
        return major;
    }
    caesarBuffer = (char*)kmalloc(memory_size*sizeof(char), GFP_KERNEL);
    xorBuffer = (char*)kmalloc(memory_size*sizeof(char), GFP_KERNEL);
    return 0;
}

void cleanup_module(void)
{
    unregister_chrdev(major, MODULE_NAME);
    kfree(caesarBuffer);
    kfree(xorBuffer);
}

int encdec_open(struct inode *inode, struct file *filp)
{
    int minor = MINOR(inode->i_rdev);
    if(minor == 0){
        filp->f_op = &fops_caesar;
    }
    else if(minor == 1){
        filp->f_op = &fops_xor;
    }
    else{
        return -ENODEV;
    }
    filp->private_data = (encdec_private_data*)kmalloc(sizeof(encdec_private_data), GFP_KERNEL);
    encdec_private_data* data = filp->private_data;
    data->key = 0;
    data->read_state = ENCDEC_READ_STATE_DECRYPT;
    return 0;
}

int encdec_release(struct inode *inode, struct file *filp)
{
    kfree(filp->private_data);
    return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    encdec_private_data* data = (encdec_private_data*)filp->private_data;
    if(cmd == ENCDEC_CMD_CHANGE_KEY){
        data->key = arg;
    }
    else if(cmd == ENCDEC_CMD_SET_READ_STATE){
        data->read_state = arg;
    }
    else if(cmd == ENCDEC_CMD_ZERO){
        char* buffer = NULL;
        int minor = MINOR(inode->i_rdev);
        if(minor == 0){
            buffer = caesarBuffer;
            caesarBufferLen = 0;
        }
        else if (minor == 1){
            buffer = xorBuffer;
            xorBufferLen = 0;
        }
        else{
            return -ENODEV;
        }
        memset(buffer, 0, memory_size);
    }
    return 0;
}

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos){
    if(*f_pos == memory_size){
        return -ENOSPC;
    }
    char* raw_data = (char*)kmalloc(count, GFP_KERNEL);
    int result = copy_from_user(raw_data, buf, count);
    if(result != 0){
        return -EFAULT;
    }
    int count_copied = 0;
    encdec_private_data* data = filp->private_data;
    unsigned char key = data->key;
    int i;
    for(i = *f_pos; i<memory_size; i++){
        if(count_copied == count){
            break;
        }
        caesarBuffer[i] = (raw_data[count_copied] + key)%128;
        count_copied++;
    }
    *f_pos += count_copied;
    caesarBufferLen += count_copied;
    kfree(raw_data);
    return count_copied;
}

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos){
    if(*f_pos == memory_size){
        return -ENOSPC;
    }
    char* raw_data = (char*)kmalloc(count, GFP_KERNEL);
    int result = copy_from_user(raw_data, buf, count);
    if(result != 0){
        return -EFAULT;
    }
    int count_copied = 0;
    encdec_private_data* data = filp->private_data;
    unsigned char key = data->key;
    int i;
    for(i = *f_pos; i<memory_size; i++){
        if(count_copied == count){
            break;
        }
        xorBuffer[i] = raw_data[count_copied]^key;
        count_copied++;
    }
    *f_pos += count_copied;
    xorBufferLen += count_copied;
    kfree(raw_data);
    return count_copied;
}

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos){
    if(*f_pos == memory_size){
        return -EINVAL;
    }
    char* buffer_data = (char*)kmalloc(count, GFP_KERNEL);
    encdec_private_data* data = filp->private_data;
    int read_state = data->read_state;
    unsigned char key = data->key;
    int count_read = 0;
    int i;
    for(i=*f_pos; i<memory_size; i++){
        if(count_read == count || i>=caesarBufferLen){
            break;
        }
        if(read_state == ENCDEC_READ_STATE_RAW){
            buffer_data[count_read] = caesarBuffer[i];
        }
        else if(read_state == ENCDEC_READ_STATE_DECRYPT){
            buffer_data[count_read] = (caesarBuffer[i] + 128 - key) % 128;
        }
        count_read++;
    }
    int result = copy_to_user(buf, buffer_data, count_read);
    if(result != 0){
        return -EFAULT;
    }
    *f_pos += count_read;
    kfree(buffer_data);
    return count_read;
}

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos){
    if(*f_pos == memory_size){
        return -EINVAL;
    }
    char* buffer_data = (char*)kmalloc(count, GFP_KERNEL);
    encdec_private_data* data = filp->private_data;
    int read_state = data->read_state;
    unsigned char key = data->key;
    int count_read = 0;
    int i;
    for(i=*f_pos; i<memory_size; i++){
        if(count_read == count || i>=xorBufferLen){
            break;
        }
        if(read_state == ENCDEC_READ_STATE_RAW){
            buffer_data[count_read] = xorBuffer[i];
        }
        else if(read_state == ENCDEC_READ_STATE_DECRYPT){
            buffer_data[count_read] = xorBuffer[i] ^ key;
        }
        count_read ++;
    }
    int result = copy_to_user(buf, buffer_data, count_read);
    if(result != 0){
        return -EFAULT;
    }
    *f_pos += count_read;
    kfree(buffer_data);
    return count_read;
}

