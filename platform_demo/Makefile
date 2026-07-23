obj-m += platform_demo.o
obj-m += platform_device_demo.o

# --- 編譯目標選擇 ---
# 預設：WSL2 x86 本機編譯
# ARM 交叉編譯：make CROSS=1
ifeq ($(CROSS),1)
    ARCH           := arm
    CROSS_COMPILE  := arm-buildroot-linux-gnueabihf-
    KDIR           := $(HOME)/linux-dev/buildroot/output/build/linux-6.18.7
else
    ARCH           :=
    CROSS_COMPILE  :=
    KDIR           := $(HOME)/linux-dev/my_module/WSL2-Linux-Kernel-linux-msft-wsl-6.6.114.1
endif

all:
	make -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
