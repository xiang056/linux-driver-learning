#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "scull.h"


static int scull_major = SCULL_MAJOR;
static int scull_minor = 0;

static struct scull_dev *scull_devices;

static int scull_open(struct inode *inode, struct file *filp)
{
    struct scull_dev *dev = container_of(inode->i_cdev, struct scull_dev, cdev);
    filp->private_data = dev;
    return 0;
}

static int scull_release(struct inode *inode, struct file *filp)
{
    return 0;
}


static ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct scull_dev *dev = filp->private_data;
    if( *f_pos >= dev->size)
        return 0;
    if(count > dev->size - *f_pos)
        count = dev->size - *f_pos;
    if(copy_to_user(buf, dev->data + *f_pos, count))
        return -EFAULT;
    *f_pos += count;
    return count;

}

static ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct scull_dev *dev = filp->private_data;
    if(dev->data == NULL)
        dev->data = kmalloc(SCULL_BUFFER_SIZE, GFP_KERNEL);
    if(!dev->data)
        return -ENOMEM;
    if (*f_pos >= SCULL_BUFFER_SIZE)
        return -ENOSPC;
    
    if(*f_pos + count > SCULL_BUFFER_SIZE){
        count = SCULL_BUFFER_SIZE - *f_pos;
        if (count == 0)
             return -ENOSPC;
    }


    if(copy_from_user(dev->data + *f_pos, buf, count))
        return -EFAULT;

    *f_pos += count;
    if(*f_pos > dev->size)
        dev->size = *f_pos;
    return count;   
}

static struct file_operations scull_fops = {
    .owner = THIS_MODULE,
    .open = scull_open,
    .release = scull_release,
    .read = scull_read,
    .write = scull_write,
};

static int scull_setup_cdev(struct scull_dev *dev, int index)
{
    int err;
    dev_t devno = MKDEV(scull_major, scull_minor + index);
    cdev_init(&dev->cdev, &scull_fops); //綁定fops
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);
    if(err)
        printk(KERN_WARNING "scull: error %d adding scull%d\n", err, index);
    return err;
}


static int __init scull_init(void)
{
    int result, i;
    dev_t dev;

    /* 動態分配 major number */
    result = alloc_chrdev_region(&dev, scull_minor, SCULL_NR_DEVS, "scull");
    if (result < 0) {
        printk(KERN_WARNING "scull: can't get major %d\n", result);
        return result;
    }
    scull_major = MAJOR(dev);

    /* 分配裝置陣列 */
    scull_devices = kmalloc(SCULL_NR_DEVS * sizeof(struct scull_dev), GFP_KERNEL);
    if (!scull_devices) {
        unregister_chrdev_region(dev, SCULL_NR_DEVS);
        return -ENOMEM;
    }
    memset(scull_devices, 0, SCULL_NR_DEVS * sizeof(struct scull_dev));

    for(i = 0; i < SCULL_NR_DEVS; i++){
        result = scull_setup_cdev(&scull_devices[i], i);
        if(result){
            for(; i >= 0; i--)
                cdev_del(&scull_devices[i].cdev);
            kfree(scull_devices);
            unregister_chrdev_region(dev, SCULL_NR_DEVS);
            return result;
        }
    }
    printk(KERN_INFO "scull: loaded, major=%d\n", scull_major);
    return 0;
}

static void __exit scull_exit(void)
{
    int i;
    dev_t dev = MKDEV(scull_major, scull_minor);
    if (scull_devices){
        for (i = 0; i < SCULL_NR_DEVS; i++){
            cdev_del(&scull_devices[i].cdev);
            kfree(scull_devices[i].data);
        }
        kfree(scull_devices);
    }

    unregister_chrdev_region(dev, SCULL_NR_DEVS);
    printk(KERN_INFO "scull: unloaded\n");
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Edward");
MODULE_DESCRIPTION("Simple scull char driver");
