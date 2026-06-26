/* ================================================================
 * gpio_test.c — user space 測試程式
 *
 * 用途：測試 simple_gpio driver 的所有 ioctl 命令
 *
 * 編譯：gcc -o gpio_test gpio_test.c
 * 執行：sudo ./gpio_test /dev/simple_gpio
 * ================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      /* open() */
#include <unistd.h>     /* close(), read(), write() */
#include <sys/ioctl.h>  /* ioctl() */

/* include 同一份 ioctl 命令定義（kernel 和 user space 共用） */
#include "simple_gpio_ioctl.h"

/* ----------------------------------------------------------------
 * 印出目前裝置狀態（用 read 介面，等同 cat /dev/simple_gpio）
 * ---------------------------------------------------------------- */
static void print_state_via_read(int fd)
{
    char buf[64] = {0};
    /* 每次 read 前要 lseek 回到開頭，否則 offset 卡在結尾回傳 EOF */
    lseek(fd, 0, SEEK_SET);
    int n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0)
        printf("  [read]  state = %s", buf);
    else
        printf("  [read]  (empty)\n");
}

/* ----------------------------------------------------------------
 * 印出目前裝置狀態（用 ioctl GET 命令）
 * ---------------------------------------------------------------- */
static void print_state_via_ioctl(int fd)
{
    int val = -1;
    int ret = ioctl(fd, GPIO_IOC_GET, &val);
    if (ret < 0) {
        perror("  [ioctl GET] failed");
        return;
    }
    printf("  [ioctl] gpio_value = %d (%s)\n", val, val ? "ON" : "OFF");
}

/* ----------------------------------------------------------------
 * main
 * ---------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    const char *dev = (argc > 1) ? argv[1] : "/dev/simple_gpio";

    printf("Opening %s ...\n", dev);
    int fd = open(dev, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    printf("OK\n\n");

    /* ---- 1. 初始狀態 ---- */
    printf("=== 初始狀態 ===\n");
    print_state_via_read(fd);
    print_state_via_ioctl(fd);
    printf("\n");

    /* ---- 2. ioctl ON ---- */
    printf("=== ioctl ON ===\n");
    if (ioctl(fd, GPIO_IOC_ON) < 0)
        perror("ioctl ON");
    print_state_via_read(fd);
    print_state_via_ioctl(fd);
    printf("\n");

    /* ---- 3. ioctl TOGGLE ---- */
    printf("=== ioctl TOGGLE (ON → OFF) ===\n");
    if (ioctl(fd, GPIO_IOC_TOGGLE) < 0)
        perror("ioctl TOGGLE");
    print_state_via_read(fd);
    print_state_via_ioctl(fd);
    printf("\n");

    /* ---- 4. ioctl SET (val = 1) ---- */
    printf("=== ioctl SET val=1 ===\n");
    int set_val = 1;
    if (ioctl(fd, GPIO_IOC_SET, &set_val) < 0)
        perror("ioctl SET");
    print_state_via_read(fd);
    print_state_via_ioctl(fd);
    printf("\n");

    /* ---- 5. ioctl OFF ---- */
    printf("=== ioctl OFF ===\n");
    if (ioctl(fd, GPIO_IOC_OFF) < 0)
        perror("ioctl OFF");
    print_state_via_read(fd);
    print_state_via_ioctl(fd);
    printf("\n");

    /* ---- 6. write 介面（原有功能驗證）---- */
    printf("=== write 'gpio_on' via echo interface ===\n");
    lseek(fd, 0, SEEK_SET);
    const char *msg = "gpio_on";
    write(fd, msg, 7);
    print_state_via_read(fd);
    print_state_via_ioctl(fd);
    printf("\n");

    close(fd);
    printf("Done.\n");
    return 0;
}
