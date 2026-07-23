#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include "blocking_io.h"


static struct blocking_dev my_dev;
static dev_t dev_num;
static struct cdev my_cdev;


ssize_t blocking_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){

	struct blocking_dev *dev = filp->private_data;

	mutex_lock(&dev->lock);

	if(dev->data_ready == 0){
		mutex_unlock(&dev->lock);  //先放鎖，再睡
		if(wait_event_interruptible(dev->read_wq, dev->data_ready != 0)){
			return -ERESTARTSYS; //被signal打斷
		}
		mutex_lock(&dev->lock); //醒來後重新上鎖
	}

	if(count > dev->data_len)
		count = dev->data_len;
	if(copy_to_user(buf, dev->buf, count)){
		mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	
	dev->data_ready = 0;
	mutex_unlock(&dev->lock);

	return count;
	
}

ssize_t blocking_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos){
	struct blocking_dev *dev = filp->private_data;
	mutex_lock(&dev->lock);

	if(count > BUF_SIZE)
		count = BUF_SIZE;
	if(copy_from_user(dev->buf, buf, count)){
		mutex_unlock(&dev->lock);
		return -EFAULT;
	}

	dev->data_len = count;
	dev->data_ready = 1;
	mutex_unlock(&dev->lock);
	wake_up_interruptible(&dev->read_wq);
	return count;
}

static __poll_t blocking_poll(struct file *filp, poll_table *wait)
{
	struct blocking_dev *dev = filp->private_data;
	__poll_t mask = 0;

	poll_wait(filp, &dev->read_wq,  wait);
	poll_wait(filp, &dev->write_wq, wait);

	mutex_lock(&dev->lock);
	if (dev->data_ready)
		mask |= EPOLLIN | EPOLLRDNORM;  // 有資料，可讀
	if (!dev->data_ready)
		mask |= EPOLLOUT | EPOLLWRNORM; // 緩衝區空，可寫
	mutex_unlock(&dev->lock);

	return mask;
}

static int blocking_open(struct inode *inode, struct file *filp){
    filp->private_data = &my_dev;
    return 0;
}

static int blocking_release(struct inode *inode, struct file *filp){
	return 0;
}

static struct file_operations blocking_fops = {
	.owner = THIS_MODULE,
	.open = blocking_open,
	.read = blocking_read,
	.write = blocking_write,
	.poll = blocking_poll,
	.release = blocking_release,
};

static int __init blocking_init(void){
	int result;

	result = alloc_chrdev_region(&dev_num, 0, 1, "blocking_io");
	if(result < 0)
		return result;

	mutex_init(&my_dev.lock);
	init_waitqueue_head(&my_dev.read_wq);
	init_waitqueue_head(&my_dev.write_wq);

	cdev_init(&my_cdev, &blocking_fops);
	my_cdev.owner = THIS_MODULE;
	result = cdev_add(&my_cdev, dev_num, 1);
	if(result){
		unregister_chrdev_region(dev_num, 1);
		return result;
	}
	printk(KERN_INFO "blocking_io: loaded, major=%d\n", MAJOR(dev_num));
	return 0;
}

static void __exit blocking_exit(void){
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev_num, 1);
	printk(KERN_INFO "blocking_io: unloaded\n");
}

module_init(blocking_init);
module_exit(blocking_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Edward");
MODULE_DESCRIPTION("Blocking I/O demo driver");