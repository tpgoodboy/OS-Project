#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#define P down_interruptible
#define V up
#define BUF_SIZE 1000

static struct cdev in_dev, out_dev;
static char buffer[BUF_SIZE+1];
static struct semaphore buf_lock;
static dev_t in_dev_no, out_dev_no;
static struct class *class_in, *class_out;
static int flag;

static ssize_t dev_write(struct file *filp, const char *buf, size_t len, loff_t *off){
	P(&buf_lock);
	if(!flag){
		size_t l = len > BUF_SIZE ? BUF_SIZE : len;
		printk("input char size is %ld\n", l);
		raw_copy_from_user(buffer, buf, l);
		flag = 1;
		V(&buf_lock);
		return l;
	}
	else{
		V(&buf_lock);
		return 0;	
	}
}

static ssize_t dev_read(struct file *filp, char *buf, size_t len, loff_t *off){
	P(&buf_lock);
	if(flag){
		size_t l = len > BUF_SIZE ? BUF_SIZE : len;
		printk("output char size is %ld\n", l);
		raw_copy_to_user(buf, buffer, l);
		memset(buffer, '\0', BUF_SIZE+1);
		flag = 0;
		V(&buf_lock);
		return l;
	}
	else{
		V(&buf_lock);
		return 0;
	}
	
}

static int dev_open(struct inode *inode, struct file *filp){
	try_module_get(THIS_MODULE);
	printk("one port opened!\n");
	return 0;
}

static int dev_close(struct inode *inode, struct file *filp){
	module_put(THIS_MODULE);
	printk("one port closed!\n");
	return 0;
}

struct file_operations in_dev_fops = {
	.write = dev_write,
	.open = dev_open,
	.release = dev_close,	
};

struct file_operations out_dev_fops = {
	.read = dev_read,
	.open = dev_open,
	.release = dev_close,	
};

static int mypipe_init(void){
	alloc_chrdev_region(&in_dev_no, 0, 1, "mypipe_in");
	cdev_init(&in_dev, &in_dev_fops);
	in_dev.owner = THIS_MODULE;
	cdev_add(&in_dev, in_dev_no, 1);
	alloc_chrdev_region(&out_dev_no, 0, 1, "mypipe_out");
	cdev_init(&out_dev, &out_dev_fops);
	out_dev.owner = THIS_MODULE;
	cdev_add(&out_dev, out_dev_no, 1);
	
	sema_init(&buf_lock, 1);
	memset(buffer, '\0', BUF_SIZE+1);
	flag = 0;
	
	class_in = class_create(THIS_MODULE, "mypipe_in");
	device_create(class_in, NULL, in_dev_no, NULL, "mypipe_in");
	class_out = class_create(THIS_MODULE, "mypipe_out");
	device_create(class_out, NULL, out_dev_no, NULL, "mypipe_out");
	
	printk("in and out devices created!\n");
	return 0;
}

void mypipe_exit(void){
	device_destroy(class_in, in_dev_no);
	class_destroy(class_in);
	cdev_del(&in_dev);
	unregister_chrdev_region(in_dev_no, 1);
	
	device_destroy(class_out, out_dev_no);
	class_destroy(class_out);
	cdev_del(&out_dev);
	unregister_chrdev_region(out_dev_no, 1);
	
	printk("in and out devices canceled!\n");
}

module_init(mypipe_init);
module_exit(mypipe_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tang Pei");
