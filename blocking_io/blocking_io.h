#ifndef BLOCKING_IO_H
#define BLOCKING_IO_H

#include <linux/wait.h>
#include <linux/mutex.h>

#define BLOCKING_MAJOR 0
#define BUF_SIZE 1024

struct blocking_dev {
	char buf[BUF_SIZE]; //資料緩衝區
	int data_len; //buf目前有幾個byte
	int data_ready; //flag: 0 = 無資料， 1= 有資料

	wait_queue_head_t read_wq; // reader睡在這裡
	struct mutex lock;
};

#endif
