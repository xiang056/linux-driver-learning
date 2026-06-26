# Linux 驅動學習筆記

跟著 LDD3 從零開始寫 kernel module。

- 環境：WSL2 Ubuntu 22.04
- 進度：LDD3 Ch1～Ch4 完成，Ch3 ioctl 擴展實作中
- 目標：Character Device → Platform Driver → QEMU ARM 實機驗證

---

## 專案列表

| 專案 | 對應章節 | 說明 | 狀態 |
|------|---------|------|------|
| [hello_module](./hello_module/) | LDD3 Ch2 | 最小 kernel module，printk / init / exit | ✅ |
| [hello_param](./hello_param/) | LDD3 Ch2 | module_param + sysfs 讀寫實測 | ✅ |
| [my_module](./my_module/) | Ch2 延伸 | 自己從頭寫的 hello module | ✅ |
| [simple_gpio](./simple_gpio/) | LDD3 Ch3 | 字元設備驅動，模擬 GPIO 讀寫 + ioctl 擴展（GET/SET/ON/OFF/TOGGLE） | 🟡 待 WSL2 實測 |
| scull | LDD3 Ch3 | 完整 scull + ioctl/lseek | ⬜ |
| platform_driver | LDD3 Ch14 | Platform Driver + Device Tree | ⬜ |
| gpio_qemu | — | QEMU ARM 上的 GPIO sysfs 驅動 | ⬜ |

---

## 環境建置

```bash
# WSL2 安裝必要套件
sudo apt install build-essential linux-headers-$(uname -r)

# 編譯任一模組
cd hello_module
make
sudo insmod hello.ko
dmesg | tail -5
sudo rmmod hello
```

---

## 學習資源

- [Linux Device Drivers, 3rd Edition（LDD3）](https://lwn.net/Kernel/LDD3/)
- [LEARNING_LOG.md](./LEARNING_LOG.md) — 每週進度與踩坑記錄
