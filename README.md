# Linux Kernel Driver 實作

以 LDD3 為主軸，在 WSL2 與 QEMU ARM 環境實測的 kernel driver 開發紀錄。

---

## 環境

| 項目 | 版本 |
|------|------|
| 開發環境 | WSL2 Ubuntu 22.04 |
| ARM 測試環境 | QEMU vexpress-a9（ARM Cortex-A9） |
| Kernel（x86） | 6.6.x（WSL2） |
| Kernel（ARM） | 6.18.7（Buildroot） |
| ARM 工具鏈 | arm-buildroot-linux-gnueabihf- |

---

## 驅動清單

### simple_gpio — 字元裝置驅動
LDD3 Ch3 核心機制實作，模擬 GPIO 讀寫。

- major/minor 動態申請、cdev 註冊
- read / write / ioctl（5 個命令：ON / OFF / TOGGLE / SET / GET）
- 實測：insmod → mknod → echo / cat → ioctl 全通過

---

### scull — 記憶體字元裝置
LDD3 官方範例，用 kernel 記憶體模擬字元裝置。

- mutex 保護共享資料、lseek 支援
- 4 個獨立裝置（/dev/scull0 ~ /dev/scull3）
- 實測：read / write / mutex / lseek 全通過

---

### blocking_io — 阻塞 I/O 驅動
LDD3 Ch6 wait_queue 機制實作。

- `wait_event_interruptible` + `wake_up_interruptible`
- poll / select / epoll 支援（`.poll` callback）
- 實測：`cat` 睡眠等待，`write` 喚醒，`select` 狀態回報正確

---

### platform_demo — Platform Driver
LDD3 Ch14 平台驅動框架實作。

- probe / remove 生命週期、`devm_` managed API
- 資源取用（`platform_get_resource`）、IRQ 申請（`devm_request_irq`）
- `of_match_table` / `MODULE_DEVICE_TABLE`
- 支援 ARM 交叉編譯（`make CROSS=1`）
- 實測：WSL2 x86 probe / remove 通過；QEMU ARM 進行中

---

## 編譯方式

```bash
# x86（WSL2 本機）
make

# ARM 交叉編譯（需 Buildroot 工具鏈）
make CROSS=1

# 載入模組
sudo insmod xxx.ko
dmesg | tail -10

# 卸載
sudo rmmod xxx
```

---

## 參考資源

- [Linux Device Drivers, 3rd Edition (LDD3)](https://lwn.net/Kernel/LDD3/)
- [LEARNING_LOG.md](./LEARNING_LOG.md) — 進度與踩坑記錄
