#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <string.h>

#define DEV "/dev/blocking_io"

int main() {
    fd_set rfds;
    struct timeval tv;
    int ret;
    char buf[64];

    int fd = open(DEV, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    const char *msg = "hello poll";
    write(fd, msg, strlen(msg));

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    ret = select(fd + 1, &rfds, NULL, NULL, &tv);
    if (ret < 0) {
        perror("select");
        return 1;
    } else if (ret == 0) {
        printf("timeout\n");
        return 0;
    }

    if (FD_ISSET(fd, &rfds)) {
        memset(buf, 0, sizeof(buf));
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n < 0) {
            perror("read");
            return 1;
        }
        printf("read: %s\n", buf);
    }

    close(fd);
    return 0;
}