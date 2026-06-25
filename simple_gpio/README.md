# simple_gpio — Linux 字元設備驅動

LDD3 第三章練習，透過 `/dev/simple_gpio` 模擬 GPIO 狀態讀寫。

## 功能

- 讀取：回傳目前 GPIO 狀態
- 寫入：接受 `gpio_on` / `gpio_off` 指令
- 動態申請裝置號碼（`alloc_chrdev_region`）
- 使用 `copy_to_user` / `copy_from_user` 安全傳遞資料

## 編譯與載入

```bash
make
sudo insmod simple_gpio.ko
MAJOR=$(cat /proc/devices | grep simple_gpio | awk '{print $1}')
sudo mknod /dev/simple_gpio c $MAJOR 0
sudo chmod 666 /dev/simple_gpio
```

## 測試

```bash
cat /dev/simple_gpio               # 讀取目前狀態
echo "gpio_on" > /dev/simple_gpio  # 設為 on
cat /dev/simple_gpio               # 應顯示：gpio_on
echo "gpio_off" > /dev/simple_gpio
```

## 卸載

```bash
sudo rmmod simple_gpio
sudo rm /dev/simple_gpio
```

## 重點概念

| 概念 | 說明 |
|------|------|
| `alloc_chrdev_region` | 動態申請 major/minor 號碼 |
| `cdev_init` / `cdev_add` | 向 kernel 註冊裝置 |
| `copy_to_user` | kernel → user 安全複製 |
| `copy_from_user` | user → kernel 安全複製 |
| `file_operations` | callback 表（open/read/write/release）|
