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

static int __init scull_init(void)
{
    int result;
    dev_t dev;

    /* 動態分配 major number */
    result = alloc_chrdev_region(&dev, scull_minor, SCULL_NR_DEVS, "scull");
    if (result < 0) {
        printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
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

    printk(KERN_INFO "scull: loaded, major=%d\n", scull_major);
    return 0;
}

static void __exit scull_exit(void)
{
    int i;
    dev_t dev = MKDEV(scull_major, scull_minor);

    if (scull_devices) {
        for (i = 0; i < SCULL_NR_DEVS; i++)
            kfree(scull_devices[i].data);
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
