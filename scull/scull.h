#ifndef SCULL_H
#define SCULL_H

#include <linux/cdev.h>

struct scull_dev {
    char *data;     /* 資料緩衝區 */
    size_t size;    /* 資料大小 */
    struct cdev cdev; /* kernel 字元裝置結構 */
};

#define SCULL_MAJOR 0   /* 預設主裝置號碼 */
#define SCULL_NR_DEVS 4   /* 預設裝置數量 */
#define SCULL_UBFFER_SIZE 4096 /* 預設緩衝區大小 */

#endif /* SCULL_H */