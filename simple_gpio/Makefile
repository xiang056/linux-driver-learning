obj-m := simple_gpio.o
KDIR := $(HOME)/linux-dev/my_module/WSL2-Linux-Kernel-linux-msft-wsl-6.6.114.1

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
