/* ================================================================
 * simple_gpio.c — 第一個 Char Device Driver
 *
 * 功能：模擬一個 GPIO 狀態裝置
 *   cat /dev/simple_gpio        → 讀出目前狀態（gpio_off 或 gpio_on）
 *   echo "gpio_on" > /dev/simple_gpio  → 寫入新狀態
 *
 * 學習目標：
 *   1. 申請裝置號碼
 *   2. 實作 file_operations（open / release / read / write）
 *   3. 向 kernel 登記 cdev
 *   4. user space ↔ kernel space 資料傳輸
 * ================================================================ */


/* ----------------------------------------------------------------
 * Header Files（標頭檔）
 *
 * Linux kernel 不能用標準 C library（stdio.h / stdlib.h），
 * 要用 kernel 自己的標頭檔。
 * ---------------------------------------------------------------- */
#include <linux/module.h>   /* module_init, module_exit, MODULE_LICENSE */
#include <linux/fs.h>       /* file_operations, alloc_chrdev_region, register */
#include <linux/cdev.h>     /* struct cdev, cdev_init, cdev_add, cdev_del */
#include <linux/uaccess.h>  /* copy_to_user, copy_from_user */
#include <linux/string.h>   /* strlen, snprintf（kernel 版本） */


/* ----------------------------------------------------------------
 * 常數定義
 * ---------------------------------------------------------------- */
#define DEVICE_NAME "simple_gpio"   /* 出現在 /proc/devices 的名字 */
#define BUF_SIZE    32              /* 狀態字串的最大長度 */


/* ----------------------------------------------------------------
 * 全域變數
 *
 * 這些變數在整個 module 生命週期都存在（insmod → rmmod）。
 * ---------------------------------------------------------------- */

/* dev_t：存放裝置號碼（major + minor 合在一起的 32-bit 數字）
 * alloc_chrdev_region 會把申請到的號碼填進這個變數 */
static dev_t dev_num;

/* struct cdev：kernel 用來代表一個 char device 的結構
 * 它把「裝置號碼」和「file_operations」綁在一起 */
static struct cdev my_cdev;

/* 模擬 GPIO 的狀態字串
 * 這就是我們的「裝置資料」，read 時送出去，write 時更新 */
static char state[BUF_SIZE] = "gpio_off\n";


/* ================================================================
 * file_operations 的實作
 *
 * 使用者每次呼叫 open / read / write / close，
 * kernel 就會來呼叫下面對應的函式。
 * ================================================================ */

/* ----------------------------------------------------------------
 * my_open — 使用者呼叫 open() 時觸發
 *
 * 參數：
 *   inode：代表 /dev/simple_gpio 這個節點本身（磁碟上的實體）
 *   filp ：代表「這一次的開啟動作」，每次 open 都是新的一個
 *
 * 這個 driver 很簡單，open 不需要做什麼，直接回傳 0（成功）。
 * ---------------------------------------------------------------- */
static int my_open(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "simple_gpio: open\n");
    /* KERN_INFO：log 等級，表示一般資訊，會出現在 dmesg */
    return 0;   /* 0 = 成功；負數 = 錯誤（例如 -ENODEV） */
}


/* ----------------------------------------------------------------
 * my_release — 使用者呼叫 close() 時觸發
 *
 * 注意：fork/dup 共用同一個 file 結構，
 *       要等所有 copy 都 close 才會呼叫 release。
 *
 * 這個 driver 沒有硬體要關閉，所以直接回傳 0。
 * ---------------------------------------------------------------- */
static int my_release(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "simple_gpio: close\n");
    return 0;
}


/* ----------------------------------------------------------------
 * my_read — 使用者呼叫 read() 時觸發（例如 cat /dev/simple_gpio）
 *
 * 參數：
 *   filp ：這次開啟的 file 結構
 *   buf  ：使用者的接收緩衝區（user space 位址，不能直接碰！）
 *   count：使用者想要讀幾個 byte
 *   offp ：目前讀到第幾個 byte（需要自己更新）
 *
 * 回傳值：
 *   > 0  → 實際讀到幾個 byte
 *   = 0  → EOF，cat 看到 0 就會停止
 *   < 0  → 錯誤
 * ---------------------------------------------------------------- */
static ssize_t my_read(struct file *filp, char __user *buf,
                       size_t count, loff_t *offp)
{
    /* 計算目前 state 字串的長度 */
    int len = strlen(state);

    /* 如果已經讀到結尾了，回傳 0（EOF）
     * cat 收到 0 就知道沒資料了，停止讀取 */
    if (*offp >= len)
        return 0;

    /* 不能讀超過剩餘資料的長度
     * 例如 state = "gpio_off\n"（9 bytes），offp=0，count=4096
     * → 實際只給 9 bytes */
    count = min(count, (size_t)(len - *offp));

    /* copy_to_user：把 kernel 的資料安全地複製到 user space
     * 語法：copy_to_user(user目的地, kernel來源, 幾個byte)
     * 回傳值：還剩幾個 byte 沒複製（0 = 全部成功）
     *
     * 為什麼不能直接 memcpy？
     * buf 是 user space 位址，在 kernel mode 直接存取會 crash */
    if (copy_to_user(buf, state + *offp, count))
        return -EFAULT;     /* EFAULT = bad address */

    /* 更新讀取位置 */
    *offp += count;

    /* 回傳實際讀到幾個 byte，cat 會把這些 byte 印出來 */
    return count;
}


/* ----------------------------------------------------------------
 * my_write — 使用者呼叫 write() 時觸發（例如 echo "gpio_on" > /dev/...）
 *
 * 參數：
 *   buf  ：使用者要寫進來的資料（user space 位址）
 *   count：使用者要寫幾個 byte
 *
 * 回傳值：
 *   > 0  → 實際寫了幾個 byte
 *   < 0  → 錯誤
 * ---------------------------------------------------------------- */
static ssize_t my_write(struct file *filp, const char __user *buf,
                        size_t count, loff_t *offp)
{
    /* 暫存從 user space 收到的資料 */
    char tmp[BUF_SIZE] = {0};

    /* 防呆：不能超過緩衝區大小 */
    if (count >= BUF_SIZE)
        return -EINVAL;     /* EINVAL = invalid argument */

    /* copy_from_user：把 user space 的資料安全地複製到 kernel
     * 語法：copy_from_user(kernel目的地, user來源, 幾個byte)
     * 回傳值：還剩幾個 byte 沒複製（0 = 全部成功） */
    if (copy_from_user(tmp, buf, count))
        return -EFAULT;

    /* 把收到的資料存進 state（去掉結尾換行，再補一個 \n） */
    snprintf(state, BUF_SIZE, "%.*s\n", (int)count, tmp);

    printk(KERN_INFO "simple_gpio: state changed to '%s'\n", state);

    /* 回傳實際寫了幾個 byte，shell 看到這個才知道寫成功了 */
    return count;
}


/* ----------------------------------------------------------------
 * file_operations 結構
 *
 * 這是一張「函式指標表」，告訴 kernel：
 *   「當使用者對我的裝置做 open/read/write/close，呼叫哪個函式」
 *
 * 沒有填的欄位（例如 .llseek）kernel 有預設處理，不用擔心。
 * ---------------------------------------------------------------- */
static struct file_operations my_fops = {
    .owner   = THIS_MODULE, /* 防止模組被卸載時還有人在使用，固定寫這個 */
    .open    = my_open,
    .release = my_release,  /* close() 對應的是 release，不是 close */
    .read    = my_read,
    .write   = my_write,
};


/* ================================================================
 * Module 初始化與清理
 *
 * __init：標記這個函式只在載入時用一次，之後 kernel 可以釋放它的記憶體
 * __exit：標記這個函式只在卸載時用
 * ================================================================ */

/* ----------------------------------------------------------------
 * gpio_init — insmod 時執行
 * ---------------------------------------------------------------- */
static int __init gpio_init(void)
{
    /* 第一步：向 kernel 申請裝置號碼（動態分配 major）
     *
     * alloc_chrdev_region(輸出dev_t, 起始minor, 要幾個, 名字)
     *
     * 成功後 dev_num 會被填入 kernel 分配的 major + minor=0
     * 名字會出現在 /proc/devices */
    alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);

    /* 第二步：初始化 cdev 結構，把它跟 fops 表綁在一起
     *
     * cdev_init(cdev結構, file_operations表) */
    cdev_init(&my_cdev, &my_fops);
    my_cdev.owner = THIS_MODULE;    /* 固定寫這個 */

    /* 第三步：向 kernel 登記這個 cdev，裝置從此上線
     *
     * cdev_add(cdev結構, 起始dev_t, 管幾個minor)
     *
     * 呼叫完這行之後，使用者就可以 open /dev/simple_gpio 了 */
    cdev_add(&my_cdev, dev_num, 1);

    printk(KERN_INFO "simple_gpio: loaded, major=%d\n", MAJOR(dev_num));
    /* MAJOR(dev_num)：從 dev_t 取出 major 號碼
     * 你等一下要用這個號碼執行 mknod */

    return 0;   /* 0 = 載入成功 */
}


/* ----------------------------------------------------------------
 * gpio_exit — rmmod 時執行（清理資源，順序與 init 相反）
 * ---------------------------------------------------------------- */
static void __exit gpio_exit(void)
{
    /* 把 cdev 從 kernel 移除（裝置下線） */
    cdev_del(&my_cdev);

    /* 釋放裝置號碼（歸還給 kernel） */
    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "simple_gpio: unloaded\n");
}


/* ----------------------------------------------------------------
 * 告訴 kernel 哪個是 init / exit 函式
 * ---------------------------------------------------------------- */
module_init(gpio_init);
module_exit(gpio_exit);
MODULE_LICENSE("GPL");  /* 必須宣告授權，否則 kernel 會警告 */
