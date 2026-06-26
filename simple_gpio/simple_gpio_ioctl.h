/* ================================================================
 * simple_gpio_ioctl.h — ioctl 命令定義
 *
 * 這個標頭檔同時被 kernel driver 和 user space 程式 include，
 * 是兩邊溝通的「合約」。
 *
 * ioctl 命令編碼規則（Linux 標準）：
 *   _IO(magic, nr)           — 不帶資料
 *   _IOR(magic, nr, type)    — 從 kernel 讀資料（Read）
 *   _IOW(magic, nr, type)    — 往 kernel 寫資料（Write）
 *   _IOWR(magic, nr, type)   — 雙向傳輸
 *
 * magic：魔數，用來區分不同 driver，避免 ioctl 號碼衝突
 *   → 選一個獨特的單字元，查 Documentation/ioctl/ioctl-number.txt
 *   → 這裡用 'G'（GPIO）
 *
 * nr：序號，同一個 driver 內的命令編號（從 0 開始）
 *
 * 每個 ioctl 命令最終編碼成一個 32-bit 數字：
 *   [方向 2bit][type 8bit][nr 8bit][size 14bit]
 * ================================================================ */

#ifndef SIMPLE_GPIO_IOCTL_H
#define SIMPLE_GPIO_IOCTL_H

/* magic number：這個 driver 專屬的識別碼 */
#define GPIO_IOC_MAGIC  'G'

/* GPIO_IOC_GET：讀取目前 GPIO 狀態     
 *   方向：kernel → user（_IOR）
 *   資料：int（0 = off, 1 = on）
 *   用法：ioctl(fd, GPIO_IOC_GET, &val); */
#define GPIO_IOC_GET    _IOR(GPIO_IOC_MAGIC, 0, int)

/* GPIO_IOC_SET：設定 GPIO 狀態
 *   方向：user → kernel（_IOW）
 *   資料：int（0 = off, 1 = on）
 *   用法：ioctl(fd, GPIO_IOC_SET, &val); */
#define GPIO_IOC_SET    _IOW(GPIO_IOC_MAGIC, 1, int)

/* GPIO_IOC_ON：強制設為 on（不需要傳資料）
 *   方向：無資料（_IO）
 *   用法：ioctl(fd, GPIO_IOC_ON); */
#define GPIO_IOC_ON     _IO(GPIO_IOC_MAGIC,  2)

/* GPIO_IOC_OFF：強制設為 off（不需要傳資料）
 *   方向：無資料（_IO）
 *   用法：ioctl(fd, GPIO_IOC_OFF); */
#define GPIO_IOC_OFF    _IO(GPIO_IOC_MAGIC,  3)

/* GPIO_IOC_TOGGLE：切換狀態（不需要傳資料）
 *   方向：無資料（_IO）
 *   用法：ioctl(fd, GPIO_IOC_TOGGLE); */
#define GPIO_IOC_TOGGLE _IO(GPIO_IOC_MAGIC,  4)

/* 命令數量上限（用於 driver 內的參數驗證）*/
#define GPIO_IOC_MAXNR  4

#endif /* SIMPLE_GPIO_IOCTL_H */
