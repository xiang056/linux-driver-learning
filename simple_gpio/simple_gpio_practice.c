/* ================================================================
 * simple_gpio_practice.c — 練習版（自己填空）
 *
 * 規則：
 *   看到 TODO 就是你要填的地方
 *   看不懂可以參考 simple_gpio.c，但先自己想
 * ================================================================ */

/* TODO 1：補上需要的 header files（4個）
 * 提示：module / fs / cdev / uaccess */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>



#define DEVICE_NAME "simple_gpio"
#define BUF_SIZE    32

/* TODO 2：宣告三個全域變數
 * 提示：裝置號碼 / cdev結構 / 狀態字串(初始值 "gpio_off\n") */
static dev_t dev_num;
static struct cdev my_cdev;
static char state[BUF_SIZE] = "gpio_off\n";



/* ----------------------------------------------------------------
 * my_open
 * 提示：回傳值代表成功或失敗
 * ---------------------------------------------------------------- */
static int my_open(struct inode *inode, struct file *filp)
{
    /* TODO 3：印出 "simple_gpio: open" 到 dmesg */
    printk(KERN_INFO "simple_gpio : open\n");
    /* TODO 4：回傳成功 */
    return 0;
}


/* ----------------------------------------------------------------
 * my_release
 * ---------------------------------------------------------------- */
static int my_release(struct inode *inode, struct file *filp)
{
    /* TODO 5：印出 "simple_gpio: close" 到 dmesg */
    printk(KERN_INFO "simple_gpio: close\n");
    /* TODO 6：回傳成功 */
    return 0;
}


/* ----------------------------------------------------------------
 * my_read
 * ---------------------------------------------------------------- */
static ssize_t my_read(struct file *filp, char __user *buf,
                       size_t count, loff_t *offp)
{
    /* TODO 7：取得 state 的長度 */
    int len = strlen(state);
    /* TODO 8：如果已經讀到結尾，回傳什麼？ */
    if (*offp >= len)
        return 0;

    /* TODO 9：count 不能超過剩餘資料長度，修正 count */
    count = min(count, (size_t)(len - *offp));

    /* TODO 10：把 state 的資料複製給 user，失敗回傳 -EFAULT */
    if (copy_to_user(buf, state + *offp, count))
        return -EFAULT;     /* EFAULT = bad address */
    /* TODO 11：更新讀取位置 offp */
    *offp += count;
    /* TODO 12：回傳實際讀了幾個 byte */
    return count;
}


/* ----------------------------------------------------------------
 * my_write
 * ---------------------------------------------------------------- */
static ssize_t my_write(struct file *filp, const char __user *buf,
                        size_t count, loff_t *offp)
{
    /* TODO 13：宣告暫存陣列 tmp[BUF_SIZE] */
    char temp[BUF_SIZE] = {0};
    /* TODO 14：防呆，count 不能超過 BUF_SIZE，超過回傳 -EINVAL */
    if (count >= BUF_SIZE)
        return -EINVAL;     /* EINVAL = invalid argument */
    /* TODO 15：把 user 的資料複製進 tmp，失敗回傳 -EFAULT */
    if (copy_from_user(temp, buf, count))
        return -EFAULT;     /* EFAULT = bad address */
    /* TODO 16：把 tmp 存進 state（用 snprintf，記得加 \n） */
    snprintf(state, BUF_SIZE, "%.*s\n", (int)count , temp);
    /* TODO 17：印出新狀態到 dmesg */
    printk(KERN_INFO "simple_gpio: state change to '%s'\n", state);
    /* TODO 18：回傳實際寫了幾個 byte */
    return count;
}


/* ----------------------------------------------------------------
 * file_operations 結構
 * ---------------------------------------------------------------- */

/* TODO 19：填入 file_operations 結構（owner / open / release / read / write） */
static struct file_operations my_fops = {
    .owner   = THIS_MODULE, /* 防止模組被卸載時還有人在使用，固定寫這個 */
    .open    = my_open,
    .release = my_release,  /* close() 對應的是 release，不是 close */
    .read    = my_read,
    .write   = my_write,
};


/* ----------------------------------------------------------------
 * gpio_init
 * ---------------------------------------------------------------- */
static int __init gpio_init(void)
{
    /* TODO 20：動態申請裝置號碼 */
    alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    /* TODO 21：初始化 cdev，綁定 my_fops */
    cdev_init(&my_cdev, &my_fops);
    /* TODO 22：設定 owner */
    my_cdev.owner = THIS_MODULE;
    /* TODO 23：向 kernel 登記 cdev */
    cdev_add(&my_cdev, dev_num, 1); 
    /* TODO 24：印出 major 號碼到 dmesg */
    printk(KERN_INFO "simple_gpio: major number %d\n", MAJOR(dev_num));

    return 0;
}


/* ----------------------------------------------------------------
 * gpio_exit
 * ---------------------------------------------------------------- */
static void __exit gpio_exit(void)
{
    /* TODO 25：裝置下線（注意順序！） */
    cdev_del(&my_cdev);
    /* TODO 26：釋放裝置號碼 */
    unregister_chrdev_region(dev_num, 1);
    /* TODO 27：印出 "simple_gpio: unloaded" 到 dmesg */    
    printk(KERN_INFO "simple_gpio: unloaded\n");
}


/* TODO 27：註冊 init / exit 函式，並宣告 LICENSE */
module_init(gpio_init);
module_exit(gpio_exit);
MODULE_LICENSE("GPL");