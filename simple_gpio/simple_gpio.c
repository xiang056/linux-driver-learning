/* ================================================================
 * simple_gpio.c — Char Device Driver + ioctl 擴展
 *
 * 功能：模擬一個 GPIO 狀態裝置
 *   cat /dev/simple_gpio              → 讀出目前狀態（gpio_off 或 gpio_on）
 *   echo "gpio_on" > /dev/simple_gpio → 用字串寫入狀態
 *   ioctl(fd, GPIO_IOC_GET, &val)     → 用 ioctl 讀取狀態（int 0/1）
 *   ioctl(fd, GPIO_IOC_SET, &val)     → 用 ioctl 設定狀態
 *   ioctl(fd, GPIO_IOC_ON)            → 強制 on
 *   ioctl(fd, GPIO_IOC_OFF)           → 強制 off
 *   ioctl(fd, GPIO_IOC_TOGGLE)        → 切換狀態
 *
 * 學習目標（本次新增）：
 *   5. 實作 unlocked_ioctl handler
 *   6. 用 _IO / _IOR / _IOW 定義 ioctl 命令
 *   7. 用 copy_to_user / put_user 回傳資料給 user space
 *   8. 用 get_user / copy_from_user 從 user space 收資料
 * ================================================================ */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>  /* copy_to_user, copy_from_user, get_user, put_user */
#include <linux/string.h>

/* ioctl 命令定義（driver 和 user space 共用的標頭） */
#include "simple_gpio_ioctl.h"


#define DEVICE_NAME "simple_gpio"
#define BUF_SIZE    32


/* ----------------------------------------------------------------
 * 全域狀態
 * ---------------------------------------------------------------- */
static dev_t        dev_num;
static struct cdev  my_cdev;

/* gpio_value：GPIO 狀態的數值表示（0 = off, 1 = on）
 * 這是 ioctl 操作的核心資料；state[] 字串則跟著同步，
 * 讓 read/write 介面繼續正常運作。 */
static int  gpio_value = 0;
static char state[BUF_SIZE] = "gpio_off\n";

/* ----------------------------------------------------------------
 * 內部 helper：同步 gpio_value → state[]
 *
 * 每次 gpio_value 改變後都呼叫一次，讓 cat 讀到的字串保持最新。
 * ---------------------------------------------------------------- */
static void sync_state(void)
{
    snprintf(state, BUF_SIZE, "%s\n", gpio_value ? "gpio_on" : "gpio_off");
}


/* ================================================================
 * file_operations 實作
 * ================================================================ */

static int my_open(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "simple_gpio: open\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "simple_gpio: close\n");
    return 0;
}

static ssize_t my_read(struct file *filp, char __user *buf,
                       size_t count, loff_t *offp)
{
    int len = strlen(state);

    if (*offp >= len)
        return 0;

    count = min(count, (size_t)(len - *offp));

    if (copy_to_user(buf, state + *offp, count))
        return -EFAULT;

    *offp += count;
    return count;
}

static ssize_t my_write(struct file *filp, const char __user *buf,
                        size_t count, loff_t *offp)
{
    char tmp[BUF_SIZE] = {0};

    if (count >= BUF_SIZE)
        return -EINVAL;

    if (copy_from_user(tmp, buf, count))
        return -EFAULT;

    /* 根據使用者輸入的字串更新 gpio_value */
    if (strncmp(tmp, "gpio_on", 7) == 0)
        gpio_value = 1;
    else if (strncmp(tmp, "gpio_off", 8) == 0)
        gpio_value = 0;
    /* 如果輸入不認識，保持原狀 */

    /* 同步 state[] 字串 */
    sync_state();

    printk(KERN_INFO "simple_gpio: write → state='%s'\n", state);
    return count;
}


/* ----------------------------------------------------------------
 * my_ioctl — 使用者呼叫 ioctl() 時觸發
 *
 * 參數：
 *   filp：這次開啟的 file 結構
 *   cmd ：ioctl 命令編號（由 _IO/_IOR/_IOW 巨集產生的 32-bit 數字）
 *   arg ：命令附帶的參數（可能是整數，也可能是 user space 指標）
 *
 * 回傳值：
 *   0 或正數 → 成功（某些命令用回傳值傳遞結果）
 *   負數     → 錯誤碼
 *
 * 為什麼是 unlocked_ioctl 而不是 ioctl？
 *   舊版 kernel 的 .ioctl 在呼叫前會取得 BKL（Big Kernel Lock）
 *   大幅降低並行效能。Linux 2.6.36 後 BKL 移除，改用
 *   .unlocked_ioctl，需要 driver 自己管理鎖（有需要的話）。
 * ---------------------------------------------------------------- */
static long my_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int val;    /* 暫存要傳給/從 user space 的整數值 */
    int ret = 0;

    /* ---- 基本驗證 ---- */

    /* 1. 檢查 magic number：確認這個 ioctl 是給我的 driver 用的
     *    _IOC_TYPE(cmd) 解碼出 magic 欄位 */
    if (_IOC_TYPE(cmd) != GPIO_IOC_MAGIC)
        return -ENOTTY;     /* ENOTTY = Inappropriate ioctl for device */

    /* 2. 檢查命令序號是否在已定義範圍內 */
    if (_IOC_NR(cmd) > GPIO_IOC_MAXNR)
        return -ENOTTY;

    /* 3. 驗證 user space 指標是否可存取（針對有資料傳輸的命令）
     *
     *    access_ok(addr, size)：
     *      - 確認 user space 這塊記憶體可以讀/寫
     *      - 只是「範圍合法」檢查，不保證頁面在記憶體中
     *      - 之後 copy_to_user / put_user 會再做頁面錯誤處理
     *
     *    _IOC_DIR(cmd) 解碼出方向欄位（READ/WRITE 從 user 視角看）
     *    VERIFY_READ  = user 空間要可讀（driver 要 copy_to_user）
     *    VERIFY_WRITE = user 空間要可寫（driver 要 copy_from_user）
     *
     *    注意：kernel 4.0+ 移除了 VERIFY_READ/WRITE，access_ok 只剩
     *    兩個參數，舊版才需要方向參數。這裡寫法適用 4.0+。 */
    if (_IOC_DIR(cmd) & _IOC_READ) {
        if (!access_ok((void __user *)arg, _IOC_SIZE(cmd)))
            return -EFAULT;
    }
    if (_IOC_DIR(cmd) & _IOC_WRITE) {
        if (!access_ok((void __user *)arg, _IOC_SIZE(cmd)))
            return -EFAULT;
    }

    /* ---- 處理各命令 ---- */
    switch (cmd) {

    /* GPIO_IOC_GET：把目前狀態傳給 user space
     *
     *   put_user(value, user_ptr)：把單一整數安全地寫到 user space
     *   比 copy_to_user 更輕量（不用指定 size，編譯器自動推導）
     *   失敗回傳 -EFAULT */
    case GPIO_IOC_GET:
        ret = put_user(gpio_value, (int __user *)arg);
        printk(KERN_INFO "simple_gpio: ioctl GET → %d\n", gpio_value);
        break;

    /* GPIO_IOC_SET：從 user space 收一個整數，更新 GPIO 狀態
     *
     *   get_user(lvalue, user_ptr)：從 user space 讀單一整數
     *   比 copy_from_user 更輕量 */
    case GPIO_IOC_SET:
        ret = get_user(val, (int __user *)arg);
        if (ret)
            break;
        gpio_value = val ? 1 : 0;  /* 正規化：任何非零都當 1 */
        sync_state();
        printk(KERN_INFO "simple_gpio: ioctl SET → %d\n", gpio_value);
        break;

    /* GPIO_IOC_ON：強制設為 on（arg 不使用） */
    case GPIO_IOC_ON:
        gpio_value = 1;
        sync_state();
        printk(KERN_INFO "simple_gpio: ioctl ON\n");
        break;

    /* GPIO_IOC_OFF：強制設為 off（arg 不使用） */
    case GPIO_IOC_OFF:
        gpio_value = 0;
        sync_state();
        printk(KERN_INFO "simple_gpio: ioctl OFF\n");
        break;

    /* GPIO_IOC_TOGGLE：切換狀態（arg 不使用） */
    case GPIO_IOC_TOGGLE:
        gpio_value = !gpio_value;
        sync_state();
        printk(KERN_INFO "simple_gpio: ioctl TOGGLE → %d\n", gpio_value);
        break;

    /* 不認識的命令 */
    default:
        ret = -ENOTTY;
        break;
    }

    return ret;
}


/* ----------------------------------------------------------------
 * file_operations 結構（加入 .unlocked_ioctl）
 * ---------------------------------------------------------------- */
static struct file_operations my_fops = {
    .owner          = THIS_MODULE,
    .open           = my_open,
    .release        = my_release,
    .read           = my_read,
    .write          = my_write,
    .unlocked_ioctl = my_ioctl,  /* ← 新增 */
};


/* ================================================================
 * Module 初始化與清理
 * ================================================================ */

static int __init gpio_init(void)
{
    alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    cdev_init(&my_cdev, &my_fops);
    my_cdev.owner = THIS_MODULE;
    cdev_add(&my_cdev, dev_num, 1);

    printk(KERN_INFO "simple_gpio: loaded, major=%d\n", MAJOR(dev_num));
    return 0;
}

static void __exit gpio_exit(void)
{
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "simple_gpio: unloaded\n");
}

module_init(gpio_init);
module_exit(gpio_exit);
MODULE_LICENSE("GPL");
