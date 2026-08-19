# Linux 驅動學習記憶 (Learning Log)

> 配合計劃表：`C:\Users\love9\Downloads\linux_learning_roadmap.md`
> 目標：嵌入式韌體 → Linux 驅動 / SoC 平台開發，年薪 +20-30%
> 起點：STM32 + embedded C，零 Linux 經驗 ｜ 預計 6-9 個月（4-5 hr/week）

---

## 📍 目前位置（每次開工先看這裡）

> 最後更新：2026-08-18（Pi 5 映像檔下載好，卡在沒有 SD 卡槽，等讀卡機到貨）

- **階段**：第三階段 — Device Tree 概念完成 → 直接開工 `stm32_linux_bridge`
- **實際時間**：第 4 週
- **進度**：Pi 5 4GB + 散熱片/風扇 + SD 卡 + 電源已備妥；ThinkPad X13 已裝好 Ubuntu 26.04 LTS（取代 WSL2 當主力開發機），交叉編譯工具鏈（build-essential/linux-headers/gcc-aarch64-linux-gnu/crossbuild-essential-arm64）與常用工具（git/vim/curl/htop/net-tools/tree/tftp-hpa）已裝完，SSH key（`~/.ssh/id_ed25519`，comment `thinkpad-to-pi5`，無 passphrase）已產生；Raspberry Pi OS Lite (64-bit) 映像檔已下載到 `~/下載/raspios_lite_arm64.img.xz`；改用手動指令（`dd` + 掛載開機分割區塞 `ssh`/`userconf.txt`/NetworkManager 設定檔）燒錄，不依賴 GUI 版 rpi-imager（snap 版本有 symbol lookup error 跑不動）；ThinkPad X13 沒有內建 SD 卡槽，已訂購 microSD 轉 USB 讀卡機，等貨到才能繼續燒錄；DT/sysfs/serdev 改為在 `stm32_linux_bridge` 專案裡現學現用，不再獨立練習
- **完成度**：約 65%
- **環境**：ThinkPad X13（Ubuntu 26.04 LTS，主力開發機，SSH 到 Pi）｜ WSL2 Ubuntu 22.04（既有交叉編譯環境，待搬遷）｜ Raspberry Pi 5 4GB（實機測試，Wi-Fi 連 `TOTOLINK_A700R_5G`）｜ 開發目錄 `~/linux-dev/`

### ▶️ 下一步要做的事（回家從這裡開始）

> **2026-08-19 再調整方向**：`stm32_linux_bridge` 主線交付物從 kernel
> `serdev` driver 改成 **user-space systemd 服務**（見 DESIGN.md 第 0
> 節）。原因：kernel driver 這塊目前還停留在「照教程走，還沒有能力
> 獨立判斷」的階段，硬做成履歷主打作品站不住腳；Application/System 層
> （daemon、systemd、穩定性、IPC）本身就是 Embedded Linux 職缺常見的
> 真實工作內容，也是現在能真正說「每一行都懂為什麼這樣寫」的範圍。
> DT/serdev/sysfs 降級為延伸項目，不是完工必要條件。

1. ~~ThinkPad 裝交叉編譯工具鏈~~ ✅ 2026-08-18 完成
2. **Pi 5 燒錄系統**（等 microSD 讀卡機到貨後從這裡繼續）：
   - 讀卡機插上 ThinkPad，`lsblk` 插卡前後各跑一次找出裝置代號（例如 `/dev/sdb`，務必確認別選錯到系統硬碟）
   - `xz -dc ~/下載/raspios_lite_arm64.img.xz | sudo dd of=/dev/sdX bs=4M status=progress conv=fsync && sync`
   - 掛載開機分割區（`/dev/sdX1`）：`touch ssh`（啟用 SSH）+ `userconf.txt`（帳號，密碼用 `openssl passwd -6` 產生 hash）
   - 掛載系統分割區（`/dev/sdX2`）：塞 `/etc/NetworkManager/system-connections/preconfigured.nmconnection`（SSID `TOTOLINK_A700R_5G` + 密碼，`chmod 600`）
   - 卸載、退卡、插 Pi 5 開機，`ping raspberrypi.local` 或 `nmap` 找 IP
   - `ssh` 用密碼登入 → `ssh-copy-id` 裝上這把公鑰（之後免密碼）：
     ```
     ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIBwQp5yTWsUX4SzuP/CWSjCg/WJUNSvYZ1nO3EN4McTh thinkpad-to-pi5
     ```
3. **確認 ThinkPad ↔ Pi 5 能 SSH 連通**
4. **確認/接線 STM32F407VG Discovery**（已有板子，沿用 Smart-Car）——Phase 0 驗證協議前需要
5. **開始 `stm32_linux_bridge` Phase 0**：Pi 上用 Python 寫協議驗證腳本，直接開
   `/dev/serial0` 收送 frame，驗證 STM32 韌體端 framing/CRC 邏輯
6. **Phase 1**：把驗證腳本改寫成長駐程式——斷線重連、例外處理、log
7. **Phase 2**：寫 `.service` unit file，systemd 開機自動啟動 + 崩潰自動重啟
8. **Phase 3**：加 Unix domain socket 介面 + CLI 工具，完整閉環 demo
9. （延伸，非必要）**Phase 4**：DT/serdev/sysfs 都真的理解後，回頭做 kernel driver 版本
10. （可選）W13-14 timer driver 實作補上

> （可選，先跳過）`poll_test.c` 加 `EPOLLOUT` + write_wq 喚醒驗證（用 fork + select 的 wfds，parent 卡住等可寫、child sleep 後讀空 buffer 觸發喚醒）。EPOLLIN 邏輯已驗證通過，EPOLLOUT 是同一段程式碼的對稱分支，風險不高，之後有餘力再補完整驗證。

---

## ✅ 進度追蹤總覽

| 階段 | 週次 | 主題 | 狀態 |
|------|------|------|------|
| 一 | W1-2 | WSL2 環境 + 基礎命令 | ✅ 完成 |
| 一 | W3-4 | 內核源碼導航 + 驅動入門 | ✅ 完成 |
| 一 | W5-6 | Character Device Driver | ✅ 完成（simple_gpio 實測通過） |
| 一 | W7-8 | ioctl 擴展 + Ch4 Debugging | ✅ 完成（ioctl 5 個命令實測通過，Ch4 讀完） |
| 二 | W9-10 | lseek + blocking I/O + scull 驅動 | ✅ 完成（scull 含 mutex/lseek 全通過） |
| 二 | W11-12 | LDD3 Ch5-6 · 中斷/異步 I/O | ✅ 完成（platform_demo + blocking_io 全通過） |
| 二 | W13-14 | LDD3 Ch7-9 · 時間/記憶體/DMA | 🟡 概念補強完成（jiffies/Timer/Workqueue/kmalloc vs vmalloc，見 2026-07-24 筆記），缺實際 timer driver 產出 |
| 二 | W15-16 | Platform Driver + Device Tree | ⬜ |
| 三 | W17-18 | ARM 環境建置 | ✅ 完成（QEMU vexpress-a9 驗證過，改用 Raspberry Pi 5 實機） |
| 三 | W19-20 | GPIO Driver + sysfs 介面 | ⬜ |
| 三 | W21-22 | UART Driver | ⬜ |
| 三 | W23-24 | 整合項目：多設備驅動框架 | ⬜ |

狀態圖例：⬜ 未開始 ｜ 🟡 進行中 ｜ ✅ 完成

---

## 📂 程式碼產出清單

| 專案 | 路徑 | 說明 | 狀態 |
|------|------|------|------|
| hello | `~/linux-dev/hello_module/` | 最小 kernel module | ✅ 已編譯 hello.ko |
| hello_param | `~/linux-dev/hello_param/` | 帶 module_param 的 module | ✅ 已 insmod 實測（int/charp/array + sysfs 0644） |
| simple_gpio | `~/linux-dev/simple_gpio/` | 字符設備驅動（LDD3 Ch3 簡化）+ ioctl 擴展 | ✅ 已實測（read/write/ioctl 5 命令全通過） |
| scull | `~/linux-dev/scull/` | LDD3 官方 scull，含 mutex + lseek | ✅ 完整實測通過（read/write/mutex/lseek） |
| timer | （待建） | 定時器驅動 | ⬜ |
| blocking_io | `~/linux-dev/blocking_io/` | 字元裝置，wait_queue 實作 blocking read/write | ✅ 完整實測通過（cat 睡眠等待 + write 喚醒） |
| platform_demo | `~/linux-dev/platform_demo/` | platform_driver 完整版（資源取用 + 中斷 + drvdata + of_match_table）| ✅ 完整實測通過 |
| platform uart | （待建） | platform_driver + device tree | ⬜ |
| gpio_sysfs | （待建） | GPIO + sysfs 接口（Pi 5 實機） | ⬜ |
| uart_char | （待建） | 字符設備版 UART 驅動 | ⬜ |

> 履歷目標：5+ 個完整驅動（character / platform / GPIO / UART）

---

## 🧠 學習筆記（隨手記，重要概念寫這裡）

### Week 1-2
- **2026-06-11** 完成 hello.ko 編譯。`hello.c` 用 `module_init` / `module_exit` 註冊進入/離開函式，`printk(KERN_INFO ...)` 輸出到 dmesg。
- **2026-06-11** 完成 `hello_param`（`module_param`）。重點：
  - `module_param(name, type, perm)`：type 有 `int` / `charp`(字串) / `bool`…；perm 是 `/sys/module/<mod>/parameters/` 下檔案權限，設 `0` 就不在 sysfs 出現，`0644` 則 root 可讀寫。
  - `module_param_array(arr, int, &count, perm)`：載入時 `arr=1,2,3`，內核自動把實際元素數填進 `count`。
  - `insmod hello_param.ko param_value=100 name="World" arr=1,2,3` 覆寫預設值；不傳就用 C 裡初值。
  - 實測：往 `/sys/.../parameters/param_value` 寫 999 → 卸載時 exit 印出 999，證明 sysfs 寫入直接改到運行中內核的變數（不是副本）。
  - `MODULE_PARM_DESC` 的描述會出現在 `modinfo` 的 `parm:` 行。
- _（待補：用戶空間 vs 內核空間隔離邊界、/dev 用途）_

### Week 3-4
- **2026-06-25** 讀完 LDD3 Ch3，完成 `simple_gpio.c`（含詳細註解版 + 填空練習版）
  - **Major/Minor number**：major 對應 driver，minor 對應同 driver 下的第幾個裝置。用 `alloc_chrdev_region` 動態申請，不要靜態指定（避免衝突）。
  - **三個重要資料結構**：
    - `file_operations`：callback 表，告訴 kernel 呼叫哪個函式（類比 STM32 的 HAL callback）
    - `struct file`：每次 open 產生一個，`private_data` 用來在 open/read/write 之間傳遞裝置資料
    - `struct cdev`：把號碼和 fops 綁在一起，`cdev_add` 後裝置上線
  - **Driver 生命週期順序**：
    - init：`alloc_chrdev_region` → `cdev_init` → `cdev_add`
    - exit：`cdev_del` → `unregister_chrdev_region`（反序！先下線再釋放號碼）
  - **user/kernel 資料傳輸**：
    - `copy_to_user`：read 時，kernel → user
    - `copy_from_user`：write 時，user → kernel
    - 不能直接 memcpy：user space 虛擬位址在 kernel mode 可能無效、記憶體可能被 swap、惡意位址安全漏洞
  - **`cat` 停止的原理**：`read()` 回傳 0 = EOF，`cat` 才會停止；不回傳 0 會無限讀下去

### Week 5-6
- **2026-06-25** 實測 `simple_gpio.ko` 完整通過
  - **`mknod` 手動建立裝置節點**：`sudo mknod /dev/simple_gpio c <major> 0`；major 號從 `dmesg` 看（本次 240）
  - **寫入需要 root**：`echo "x" > /dev/simple_gpio` 會 Permission denied，要用 `echo "x" | sudo tee /dev/simple_gpio`
  - **踩坑：`simple_gpio` Makefile 的 KDIR 指向 `/lib/modules/.../build`**，該連結不存在；改成指向 `~/linux-dev/my_module/WSL2-Linux-Kernel-linux-msft-wsl-6.6.114.1` 才能編譯（與 `my_module` 相同路徑）

### Week 7
- **2026-06-26** 閱讀 LDD3 Ch4（Debugging Techniques）
  - **Ch4 定位**：工具層，不是底層知識。Ch3 driver 出問題時才真的用得到，現階段讀過知道有這些工具即可，遇到問題再回來查
  - **printk log level**：8 級（EMERG=0 最嚴重 → DEBUG=7），`console_loglevel` 決定哪些印到畫面，其餘只進 dmesg
  - **可開關 debug 巨集**：用 `#ifdef SCULL_DEBUG` 包 `PDEBUG`，Makefile 加 `-DSCULL_DEBUG` 開啟，release 版不用改程式碼
  - **printk_ratelimit()**：避免錯誤發生時每秒噴幾千行 log
  - **查詢系統狀態**：`/proc`（簡單但不推薦）、`ioctl`（快、binary）、`sysfs`（現代推薦，Ch14）
  - **strace**：看 user space 所有 system call 的參數和回傳值，可確認 driver 行為正不正確
  - **oops 訊息**：看 `EIP is at 函式名+offset [module]` → 找出問題函式；Call Trace 往下追呼叫鏈
  - **System hang**：在迴圈裡插 `schedule()` 讓其他 process 搶 CPU；Magic SysRq 緊急救援
  - **gdb**：`gdb vmlinux /proc/kcore` 可看 kernel 變數，但不能設 breakpoint 也不能改資料
  - **心得**：Ch4 在沒遇過 bug 的時候讀很抽象，等實測出問題再回來看會快很多

### Week 8
- **2026-06-28** 建立 `scull` 驅動骨架（LDD3 Ch3 標準範例）
  - **scull 是什麼**：Simple Character Utility for Loading Localities，用 kernel 記憶體模擬字元裝置，沒有真實硬體。write 存進 kmalloc buffer，read 從 buffer 讀回，像住在 kernel 裡的記事本
  - **為什麼學 scull**：涵蓋 char driver 所有核心機制（major/minor、cdev、file_operations、copy_to/from_user、kmalloc），真實硬體驅動結構相同，只是把記憶體換成暫存器
  - **骨架包含**：
    - `scull.h`：定義 `scull_dev`（data buffer + size + cdev），4 個裝置 (`SCULL_NR_DEVS=4`)，buffer 4096 bytes
    - `scull_init`：`alloc_chrdev_region` 動態申請 major → `kmalloc` 分配 4 個裝置陣列 → `memset` 清零
    - `scull_exit`：逐一 `kfree` 各裝置 data → `kfree` 裝置陣列 → `unregister_chrdev_region`
  - **實測結果**：編譯成功，insmod 取得 major=240，mknod 建立 `/dev/scull0~3`；cat/echo 回傳 "No such device or address"（正常，因為 `file_operations` 尚未實作）
  - **下一步**：實作 `open` / `read` / `write` / `release` callback，讓 `/dev/scull0` 真正能讀寫

### Week 9

- **2026-07-06** 完成 platform_demo 資源取用（`platform_get_resource`）
  - **`platform_get_resource(pdev, IORESOURCE_MEM, 0)`**：從 platform_device 取出第 0 個 MEM 資源，回傳 `struct resource *`（含 start/end/flags）
  - **`devm_ioremap_resource` vs `devm_ioremap`**：前者會先 `request_mem_region` 搶佔位址再映射；後者直接映射不搶佔。WSL2 x86 上 0x10000000 是 RAM 區域，`request_mem_region` 失敗 → 改用 `devm_ioremap` 也失敗（RAM 不能 ioremap）
  - **WSL2 限制**：ioremap 只對 MMIO（外設暫存器）位址有效，不能對 RAM 位址使用。真實 ARM SoC 上 peripheral 位址（如 0x3F200000）才是正確目標，QEMU 上再實作
  - **resource_size(res)**：計算資源大小 = `res->end - res->start + 1`，比手動算安全

- **2026-06-30** 完成 Platform Driver 骨架實測（platform_demo）
  - **Platform Driver 定位**：驅動焊死在 SoC 上的硬體（UART/GPIO/I2C），硬體無法自動偵測，需透過 Device Tree 描述
  - **Device Tree**：硬體清單，描述裝置位址/中斷/compatible 字串；kernel 啟動時解析，建立 platform_device
  - **compatible 配對**：DTS 的 `compatible` 字串 vs driver 的 `of_device_id` 表，完全一樣才配對成功，kernel 呼叫 `probe()`
  - **probe vs init**：`probe` 是 kernel 配對成功後才呼叫（每個裝置各一次），`pdev` 帶有 DTS 資源；`init` 是 insmod 直接跑，無硬體資訊
  - **取資源流程**：`platform_get_resource()` 取實體位址 → `devm_ioremap_resource()` 映射成虛擬位址 → 才能讀寫暫存器
  - **為何需要 ioremap**：Linux 有 MMU，kernel 跑在虛擬位址空間，不能直接用實體位址（不同於 STM32 直接操作實體位址）
  - **devm_ 系列**：managed API，`remove` 時 kernel 自動釋放，不需要手動清理
  - **建立 platform_demo 骨架**：4 個檔案（platform_demo.h / platform_demo.c / platform_device_demo.c / Makefile），device 和 driver 分兩個 .ko，手動模擬 DTS 配對

### Week 11

- **2026-07-22** 完成 QEMU ARM 環境建置（Buildroot 全套編譯 + vexpress-a9 開機成功）
  - **為什麼要這步**：WSL2 是 x86_64，之前 platform_demo 的 `devm_ioremap` 一直卡在「0x10000000 是 RAM 位址不能 ioremap」；要驗證真實硬體驅動邏輯，必須換到真正的 ARM 環境
  - **Buildroot 流程**：`git clone buildroot` → `make qemu_arm_vexpress_defconfig` → `make`，一次編出交叉編譯工具鏈（`arm-buildroot-linux-gnueabihf-`）、Linux kernel（`zImage`）、根檔案系統（`rootfs.ext2`）、device tree（`vexpress-v2p-ca9.dtb`）
  - **踩坑：WSL2 PATH 汙染**：WSL2 會把 Windows PATH（含空格路徑如 `C:\Program Files\...`）自動附加進 Linux PATH，Buildroot 嚴格檢查禁止 PATH 含空格，直接編譯會報 `dependencies.mk Error 1`。解法：編譯時用乾淨 PATH，`PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin make -j$(nproc)`
  - **踩坑：host-cmake bootstrap 連結失敗**：`-j16` 高平行度下 CMake 自帶的 bootstrap 腳本出現 `undefined reference to cmsys::SystemToolsManager` 這類連結錯誤（已知的平行編譯 race condition）。解法：`make host-cmake-dirclean` 清掉重編，之後改用較低的 `-j4`
    避免。
  - **背景編譯與工作階段的坑**：用 `nohup ... &` 讓編譯在背景跑，如果外層指令本身直接返回（沒有保持 attach），這個背景行程可能因為工作階段中斷而跟著消失，而不是編譯本身出錯。要嘛讓外層指令保持前景阻塞（讓工具自己的背景執行機制追蹤整個生命週期），要嘛正確做好 `disown` 讓行程完全脫鉤。
  - **開機驗證**：`./start-qemu.sh --serial-only`，QEMU 模擬 vexpress-a9（ARM Cortex-A9）開機，完整跑過 kernel 初始化、EXT4 掛載 rootfs、`udhcpc` 拿到網路，最後進入 `buildroot login:`，`root` 免密碼登入
  - **`uname -a`** 確認 `armv7l`，證實這是真正的 ARM 架構，不是 x86 模擬
  - **下一步**：要用 Buildroot 產生的交叉編譯工具鏈重新編譯 `platform_demo.ko`（對應這份 `linux-6.18.7` kernel source），才能在 QEMU 裡 `insmod` 測試

- **2026-07-22** 完成 `blocking_io` driver（LDD3 Ch06 blocking I/O 實作）
  - **核心機制**：`wait_event_interruptible(wq, condition)` + `wake_up_interruptible(&wq)` 是一對，前者讓 process 睡到條件成立為止（真正睡眠，不占 CPU，不同於 `while(!ready);` 忙等），後者由另一個 process 呼叫，喚醒睡在該 wait_queue 上的所有 process
  - **關鍵陷阱：睡眠前必須先 unlock**：`mutex_lock` 之後如果直接呼叫 `wait_event_interruptible` 睡著，鎖不會被自動釋放，會造成死鎖（因為能喚醒 reader 的 writer 也需要搶同一把鎖，卻進不來）。正確順序：`mutex_unlock` → `wait_event_interruptible` → 醒來後 `mutex_lock`
  - **`wait_event_interruptible` 回傳值**：非 0 代表睡眠被 signal 打斷（如 Ctrl+C），要回傳 `-ERESTARTSYS`，不是回傳資料本身

- **2026-07-23** blocking_io 加上 poll/select 支援（`.poll` callback）
  - **為什麼需要 poll**：blocking read 一次只能顧一個 fd；同時監聽多個裝置時（如 UART + sensor），需要 `select`/`poll`/`epoll`，哪個 fd 有資料就處理哪個，不會互相卡住
  - **user 呼叫 `select()`，kernel 呼叫 driver 的 `.poll`**：driver 回傳 bitmask（`EPOLLIN`=可讀、`EPOLLOUT`=可寫），kernel 再告訴 user 哪個 fd 準備好
  - **`poll_wait()` 不會睡**：只是把 wait_queue 登記給 kernel 監聽，真正等待是 kernel 的事；driver `.poll` 本身只負責「回報狀態」，不改任何 flag
  - **platform_demo Makefile 新增 ARM 交叉編譯**：`make CROSS=1` 切換到 `arm-buildroot-linux-gnueabihf-` 工具鏈 + `linux-6.18.7` kernel source；`make` 維持原本 WSL2 x86 編譯
  - **`blocking_read`/`blocking_write` 對稱設計**：read 沒資料就睡、有資料才 `copy_to_user` 並清空 `data_ready`；write 把資料 `copy_from_user` 存進 buffer、設定 `data_ready=1`，最後呼叫 `wake_up_interruptible` 叫醒睡著的 reader
  - **實測驗證**：終端機 A 執行 `cat /dev/blocking_io` 卡住（真睡眠，非忙等）；終端機 B `echo | tee` 寫入後，A 立刻印出資料，證實 wait_queue 機制運作正常
  - **踩坑**：`.c` 檔一開始只 include 了 `linux/module.h`，導致 `struct file`、`file_operations`、`copy_to/from_user`、`cdev` 系列全部找不到定義；補上 `linux/kernel.h`、`linux/fs.h`、`linux/cdev.h`、`linux/uaccess.h` 才編譯過

- **2026-07-24** Linux 核心概念補強（公司純 Windows 環境，純概念學習）

  **Linux 整體架構：**
  - User Space → syscall 邊界 → VFS → Driver 層 → Hardware，由上到下
  - User 不能直接碰硬體，只能透過 syscall（open/read/write/ioctl）進入 kernel
  - VFS 把所有東西當檔案，`/dev/xxx` 看起來像檔案，背後是你寫的 driver

  **六個核心概念重新釐清：**
  - **`filp->private_data`**：open/read/write 三個函式之間共享資料的橋，open 存進去，read/write 取出來
  - **`container_of`**：kernel 只給你結構體內某個欄位的位址，往前算 offset 找出整個結構體起點
  - **mutex + wait_event 順序**：睡眠前必須先 unlock，不然 writer 搶不到鎖永遠叫不醒 reader（死結）
  - **`copy_to_user`**：user/kernel 是兩個隔離的記憶體空間，不能直接 memcpy，必須透過這個函式跨越邊界
  - **`probe` vs `init`**：init 是 insmod 時跑；probe 是 kernel 配對到硬體後才呼叫，帶有硬體資源
  - **`wake_up_interruptible`**：由 writer 呼叫，通知 kernel 喚醒睡在 wait_queue 的 reader

  **Process vs IRQ context：**
  - Process context（read/write/probe）：可以睡眠，排程器可以介入
  - IRQ context（irq_handler/timer_callback）：不能睡眠，CPU 正在處理中斷，一睡整個系統凍結
  - IRQ handler 只做最少的事（讀暫存器、設 flag、wake_up），複雜工作丟給 Workqueue 在 process context 執行

  **Ch7 Timer / Workqueue：**
  - `jiffies`：kernel 開機後每個 tick +1，HZ=1000 代表 1秒=1000 jiffies
  - Timer：設定幾秒後觸發 callback，不占 CPU；callback 在 IRQ context，不能睡眠
  - Workqueue：背景執行，跑在 process context，可以睡眠；Timer 裡需要複雜工作時用 `schedule_work` 丟給它

  **虛擬記憶體 / 實體記憶體：**
  - 實體記憶體：RAM 真實硬體位址，只有一份
  - 虛擬記憶體：每個 process 各自看到的假位址空間，透過 MMU 翻譯成實體位址
  - Page Table：每個 process 各自一份對應表（虛擬→實體），CPU 切換 process 時同時切換對應表
  - User space：每個 process 各自獨立（虛擬位址相同，實體位址不同）
  - Kernel space：所有 process 共用（虛擬位址相同，實體位址也相同）
  - 意義：多個 process 同時執行互不干擾；RAM 不夠時可 swap 到硬碟

  **kmalloc vs vmalloc：**
  - `kmalloc`：實體連續，速度快，限制約 4MB，99% 的 driver 用這個
  - `vmalloc`：虛擬連續實體不連續，可分配大塊記憶體，速度慢，DMA 不能用
  - 選擇原則：小於 1MB 用 kmalloc，大於 1MB 用 vmalloc，DMA 用 dma_alloc_coherent

- **2026-08-18** ThinkPad X13 交叉編譯工具鏈裝好 + 開始 Pi 5 燒錄，卡在沒有 SD 卡槽
  - **工具鏈**：`build-essential`、`linux-headers-$(uname -r)`、`gcc-aarch64-linux-gnu` / `crossbuild-essential-arm64`、`git`/`vim`/`curl`/`htop`/`net-tools`/`tree`/`tftp-hpa` 全部裝完；`apt install tftp` 找不到套件，官方已改名 `tftp-hpa`（廢棄套件被取代很常見，遇到就找 apt 的替代建議）
  - **SSH key**：`ssh-keygen -t ed25519` 產生 `~/.ssh/id_ed25519`（無 passphrase），public key 固定不變，只要私鑰檔案沒被覆蓋/刪除就能一直重複用
  - **rpi-imager GUI 版跑不動**：snap 版本 `symbol lookup error: undefined symbol __libc_pthread_init`（函式庫版本衝突），改用純指令流程繞過：`curl` 下載官方最新映像 `https://downloads.raspberrypi.com/raspios_lite_arm64_latest`（500MB）→ 之後 `xz -dc | sudo dd` 直接寫卡 → 手動掛開機分割區塞 `ssh` 空檔（開 SSH）+ `userconf.txt`（帳密，密碼要先用 `openssl passwd -6` 產生 hash）→ 掛系統分割區塞 `/etc/NetworkManager/system-connections/*.nmconnection`（Bookworm 已改用 NetworkManager，不再是 wpa_supplicant）設定 Wi-Fi
  - **找 SSID 免用手機翻**：`nmcli device status` 直接看目前連線中的 Wi-Fi 名稱（本機是 `TOTOLINK_A700R_5G`），密碼可用 `sudo nmcli -s -f 802-11-wireless-security.psk connection show "<SSID>"` 取回已存的
  - **卡住**：ThinkPad X13 兩側都沒有內建 SD 卡槽（只有 USB-A/USB-C），microSD 卡本身沒問題（ADATA 64GB，內附 SD 轉卡），但轉卡只是變大不是變 USB，仍需要另外買 microSD 轉 USB 讀卡機才能讓筆電讀到卡——已下單等貨到
  - **下一步**：讀卡機到貨後接著跑 `lsblk`（插卡前後各一次找裝置代號）→ `dd` 寫卡 → 塞設定檔 → 開機找 IP → SSH 進去

- **2026-08-14** 調整學習策略：DT/sysfs/serdev 改為在 `stm32_linux_bridge` 裡現學現用
  - **問題**：獨立練習 W15-16 DT overlay、W19-20 sysfs 這種抽象範例，學完沒有立刻用在真實目標上，容易忘
  - **調整**：不再要求「先把計劃表走完再開工」，改成直接開始 `stm32_linux_bridge`，DT overlay 綁在 Phase 1 現學、sysfs 綁在 Phase 2 現學，卡住當場查
  - **限制沒變**：`serdev` 裝置綁定機制本身依賴 Device Tree（`compatible` 配對觸發 `probe()`），這塊無法跳過，只是不用先做孤立練習，直接在 Phase 1 寫這個專案的 overlay 就是在學
  - **硬體到位**：Raspberry Pi 5 4GB + 散熱片風扇 + microSD 卡 + 27W USB-C PD 電源已備妥，下一步是確認 STM32 開發板，然後直接開始 Phase 0

- **2026-07-27** 草擬 `stm32_linux_bridge/DESIGN.md`——履歷作品構想
  - **目標**：STM32 韌體 + Linux `serdev` driver 的 coprocessor 系統，同時展示韌體與 kernel 兩邊能力，作為轉職履歷招牌作品
  - **架構決定**：選 `serdev` 子系統（UART 接 MCU 的正確 kernel 框架，藍牙 HCI UART、GPS receiver 都用這套），不是 user space 直接讀 tty
  - **前置依賴**：`serdev` 的 Device Tree 綁定邏輯跟目前在學的 platform driver 相通，但 Device Tree 本身（W15-16）跟 sysfs 介面設計（W19-20）都還沒實測過——決定先把這兩塊計劃表走完，再回頭做這個 project，避免 DT + serdev 兩個新概念同時疊加卡關
  - **狀態**：純設計草稿，尚未開工

- **2026-07-29** Device Tree 基礎概念（公司 Windows 環境，純概念學習）
  - **DT 用途**：描述硬體給 kernel，不把位址寫死在 driver code 裡；kernel 啟動時讀進來，找對應 driver 呼叫 probe()
  - **compatible 配對**：DTS 的 `compatible` 字串跟 driver 的 `of_device_id` 完全一樣才配對成功；可以放多個字串，kernel 從左到右找第一個符合的 driver（向後相容用途）
  - **DTS vs DTB**：`.dts` 是人看的文字檔，`dtc` 編譯成 `.dtb`（二進位），kernel 開機時讀 `.dtb`
  - **Overlay**：不動原本 DTB，只疊加新增部分；編譯成 `.dtbo`，樹莓派在 `config.txt` 加 `dtoverlay=xxx` 載入；開發自訂裝置時用這個，不需要重編整個 DTB
  - **節點**：`{ }` 區塊，可巢狀；`label: name@address { }` 格式，有 label 才能用 `&label` 引用
  - **`&` 引用**：overlay 裡用 `&uart0` 引用主 DTB 的節點，在它底下加子節點（serdev 裝置掛在 uart 底下就是這樣做）
  - **常用屬性**：`reg`（位址+大小，platform_get_resource 讀這個）、`interrupts`（platform_get_irq 讀這個）、`status`（okay/disabled）、`clocks`、`pinctrl`
  - **屬性型別**：`<>` 是整數、`""` 是字串、單獨一個屬性名代表 bool true

- **2026-07-28** 完成 `poll_test.c`——user space select 測試程式（公司 Windows 環境純寫碼）
  - **select 流程**：`FD_ZERO` 清空集合 → `FD_SET` 把 fd 放進去 → `tv` 設逾時 → `select()` 等待 → `FD_ISSET` 確認哪個 fd 觸發
  - **fd 是整數**：`open()` 回傳一個編號（整數），0/1/2 被 stdin/stdout/stderr 佔用，第一個自開的 fd 通常是 3；之後 read/write/close 都靠這個號碼操作
  - **`nfds = fd + 1`**：select() 內部從 0 掃到 nfds-1，要告訴它掃到哪裡停；監聽多個 fd 時填最大值 + 1
  - **`struct timeval`**：`tv_sec`=秒、`tv_usec`=微秒；select() 回傳 0 = 逾時，>0 = 有 fd 就緒，<0 = 出錯
  - **`FD_ZERO/FD_SET/FD_ISSET`** 是巨集，不需要背，記住用途：清空、加入、判斷
  - **`const char *msg = "hello poll"`**：字串字面值放在 `.rodata`（唯讀區），`msg` 是指向它的指標；`const` 表示不能修改
  - **`perror("open")`**：印出 `open: No such file or directory` 這類人看得懂的錯誤，自動讀 errno 翻譯
  - **測試邏輯**：write 寫資料進 driver（data_ready=1）→ select 問可讀嗎（kernel 呼叫 .poll 回傳 EPOLLIN）→ FD_ISSET 確認 → read 讀出並印出
  - **回家驗證**：`gcc -o poll_test poll_test.c` → `sudo insmod blocking_io.ko` → `mknod` → `./poll_test`，預期印出 `read: hello poll`

- **2026-07-28** `platform_demo` 在真實 ARM（QEMU vexpress-a9）環境首次實測，兩個真實硬體才會出現的 bug
  - **交叉編譯環境設定**：Buildroot 產生的工具鏈在 `~/linux-dev/buildroot/output/host/bin/`，要 `export PATH` 才能讓 `make CROSS=1` 找到 `arm-buildroot-linux-gnueabihf-gcc`（加進 `.bashrc` 才會每次自動生效）
  - **把 `.ko` 塞進 QEMU 的方法**：`rootfs.ext2` 是一個完整的 ext2 映像檔，開機前先在 WSL host 端 `sudo mount -o loop rootfs.ext2 /tmp/xxx`，把 `.ko` 複製進 `/root/`，`umount` 後再開機——**QEMU 開著的時候不能同時 mount 修改同一個映像檔**，容易造成檔案系統損毀
  - **`request_irq(80)` 回傳 `-EINVAL`（bug 1）**：手動用 `platform_device_register()` 註冊裝置、憑空填一個 IRQ 號碼（不管填 13 還是換算過的 80），在真實 kernel 上都會失敗。原因是現代 kernel 用 **sparse IRQ**（開機 log `NR_IRQS: 16, nr_irqs: 16` 就是證據），Linux 的 irq 號碼是**執行時動態分配**的，只有裝置真的透過 device tree 的 `interrupts` 屬性被 `irq_of_parse_and_map()` 解析過，才會有合法的 virq——沒有 DT 綁定，憑空填的號碼永遠對應不到 `irq_desc`。這代表要真正測試 request_irq，必須先學會 Device Tree overlay（W15-16），單純改 header 裡的數字治標不治本
  - **`devm_ioremap_resource` 回傳 `-EBUSY`（bug 2，也是這次真正驗證到的目標）**：`DEMO_MEM_START=0x10000000` 在真實 vexpress-a9 上是合法的 MMIO 位址，但已經被別的系統 driver（推測是 `dcc`/`v2m_sysreg`）佔用，`request_mem_region` 偵測到衝突直接擋下來。**這跟之前 WSL2 x86 上「0x10000000 是 RAM 不能 ioremap」是不同層級的失敗**——x86 是架構不支援，真實 ARM 是資源衝突，證明 `devm_ioremap_resource` 的保護機制在真實硬體上確實有效
  - **probe 容錯設計**：讓 `demo_probe` 對 IRQ 失敗只印警告、不 `return`，才能繼續往下測 MEM resource，一次 probe 失敗不代表整個測試都要中斷

- **2026-07-27** 重新編譯 `blocking_io`（確認 poll 支援無編譯錯誤），並修正 poll 對稱性 bug
  - **`poll_wait()` 只是登記，不是等待**：把目前 process 掛到指定 wait_queue 上，讓 kernel 之後知道要監聽哪個 queue；真正的睡眠/喚醒仍要靠 `wait_event_interruptible` / `wake_up_interruptible`
  - **對稱性陷阱**：`blocking_write` 寫完會 `wake_up_interruptible(&dev->read_wq)`，但 `blocking_read` 讀完只把 `data_ready` 設回 0，卻忘了對稱地 `wake_up_interruptible(&dev->write_wq)`——導致睡在 `write_wq` 上等 `EPOLLOUT` 的 process 永遠不會被喚醒（除非被 signal 打斷）。有 wait_queue 就要找到「誰負責在對應時機呼叫 wake_up」，兩邊角色都要检查，不能只顧其中一個方向
  - **下一步**：寫 C 測試程式用 `poll()` 實測 `EPOLLIN`/`EPOLLOUT` 是否正確回報

- **2026-06-30** 深入理解 scull 指標機制（透過考題練習）
  - **`filp->private_data` 橋接機制**：`open` 用 `container_of` 找到正確的 `scull_dev`，把位址存進 `filp->private_data`；`read`/`write` 直接從 `filp` 取回，不需要重新搜尋。4 個裝置各自有 `filp`，靠這個機制區分操作的是哪一個
  - **`container_of` 數學**：`scull_dev 起點 = i_cdev 位址 - cdev 的 offset`；offset 算法 = 前面所有欄位大小相加（data:8 + size:8 + lock:40 = 56，所以 cdev offset = 56）
  - **`dev->data` 兩層指標**：`dev->data` 欄位本身住在結構起點（offset 0），裡面存的是 `kmalloc` 回傳的位址；實際資料在那個位址，不在結構裡
  - **`copy_to_user` 來源位址**：`dev->data + *f_pos`，是用 `dev->data` 裡存的值（kmalloc 位址）加偏移，不是 `dev->data` 欄位本身的位址加偏移
  - **完整 write 流程**：`open` → `container_of` 找裝置 → `filp->private_data = dev` → `write` 取回 `dev` → `kmalloc`（第一次）→ `copy_from_user` → 更新 `dev->size`
  - **LDD3 Ch05 重點整理**：spinlock vs mutex（spinlock 忙等不能睡，mutex 可以睡）；scull 用 mutex；競態條件範例（兩個 process 同時過 `if (!dev->data)` → 記憶體洩漏）；現代 API：`mutex_init` / `mutex_lock` / `mutex_unlock`（書上舊版是 `down_interruptible` / `up`）

- **2026-06-29** 實作 `scull` 的 `file_operations`（open / read / write / release）
  - **`scull_open`**：用 `container_of(inode->i_cdev, struct scull_dev, cdev)` 從 kernel 給的 `cdev` 反推出整個 `scull_dev`，存進 `filp->private_data` 供後續函式使用
  - **`container_of` 原理**：kernel 只給你結構體內某個欄位的地址，`container_of` 用欄位的偏移量往前算，找出整個結構體的起始地址
  - **`private_data` 的用途**：`open`/`read`/`write`/`release` 是四個分開的函式，`filp->private_data` 是它們之間共享資料的橋樑
  - **`scull_read`**：三步驟 — ① `*f_pos >= dev->size` 回傳 0（EOF）② 截斷超出範圍的 count ③ `copy_to_user` 複製資料，失敗回傳 `-EFAULT`
  - **`scull_write`**：lazy allocation — 第一次寫入才 `kmalloc`，`copy_from_user` 後更新 `dev->size`
  - **指標重點**：`f_pos` 用指標傳入是因為 C 是值傳遞，要讓函式真的改到游標位置，必須傳地址（`*f_pos += count` 才能讓外面的值更新）
  - **`scull_setup_cdev`**：`cdev_init` 綁定 fops → `cdev_add` 註冊裝置，失敗印 WARNING
  - **修正 typo**：`scull.h` 的 `SCULL_UBFFER_SIZE` → `SCULL_BUFFER_SIZE`

- **2026-06-26** 實作 `simple_gpio` ioctl 擴展
  - **ioctl 命令定義巨集**：
    - `_IO(magic, nr)` — 不傳資料（例如 ON/OFF/TOGGLE）
    - `_IOR(magic, nr, type)` — kernel → user（GET）
    - `_IOW(magic, nr, type)` — user → kernel（SET）
    - 產生 32-bit 編號，包含方向、magic、序號、資料大小
  - **magic number**：選一個字元區分 driver，避免不同 driver 的 ioctl 號碼衝突
  - **handler 命名**：`.unlocked_ioctl`，舊版 `.ioctl` 需要 BKL，2.6.36 後廢除
  - **參數驗證三步驟**：① `_IOC_TYPE` 檢查 magic → ② `_IOC_NR` 檢查序號上限 → ③ `access_ok` 驗證 user 指標合法
  - **put_user / get_user**：傳單一整數比 `copy_to_user`/`copy_from_user` 更輕量（不需要手動指定 size）
  - **共用標頭**：`simple_gpio_ioctl.h` 同時被 kernel 和 user space include，是兩邊的「合約」
  - 新增 `gpio_test.c` user space 測試程式，測試全部 5 個 ioctl 命令
- **2026-06-27** 實測 `simple_gpio` ioctl 擴展完整通過
  - 固定實測流程：`make` → `insmod` → `dmesg` 拿 major → `mknod` → `gcc gpio_test` → `./gpio_test` → `dmesg` 確認 → `rmmod` + `rm /dev/`
  - dmesg 確認每個 ioctl 命令都有對應 kernel log（ON/OFF/TOGGLE/SET/GET）
  - `[read] (empty)` 原因：ioctl 改變狀態後沒有更新字串緩衝區，功能正確但可改進

<!-- 之後每週往下加，格式：日期 + 學到的關鍵點 / 踩到的坑 -->

---

## ⚠️ 踩坑記錄（出錯就記，避免重蹈）

| 日期 | 問題 | 原因 | 解法 |
|------|------|------|------|
| 2026-06-11 | `make` 報 `/lib/modules/$(uname -r)/build: No such file` | WSL2 內核更新後，`build` 符號連結掉了 | `sudo ln -sfn /usr/src/wsl2-headers-$(uname -r) /lib/modules/$(uname -r)/build` |
| 2026-06-11 | modpost 報 `_printk`/`module_layout` undefined | 內核源碼樹的 `Module.symvers` 是 0 bytes（只做過 modules_prepare，沒完整編過內核），`CONFIG_MODVERSIONS=y` 解析不到符號 | 在源碼樹 `make -j$(nproc)` 完整編一次內核 → 產生齊全的 Module.symvers（一次性，之後所有模組都能用） |
| 2026-06-11 | `insmod` 預期會報 version magic 不匹配 | 本地重編的 vermagic 結尾多一個 `+`（git 樹 + `CONFIG_LOCALVERSION_AUTO` off 時 `scm_version --short` 會加），運行內核沒有 `+` | build 時設 `LOCALVERSION=`（空但已設定），跳過 setlocalversion 加 `+` 的分支；重生 `kernel.release`/`utsrelease.h` 後重編模組 |
| 2026-06-25 | `simple_gpio` make 報 `/lib/modules/.../build: No such file` | Makefile KDIR 指向系統 build 符號連結，WSL2 沒有對應 headers | 改 KDIR 指向已完整編譯的 kernel source：`~/linux-dev/my_module/WSL2-Linux-Kernel-linux-msft-wsl-6.6.114.1` |
| 2026-06-25 | `echo "gpio_on" > /dev/simple_gpio` 報 Permission denied | shell 重導向由目前 user 執行，不繼承 sudo 權限 | 改用 `echo "gpio_on" \| sudo tee /dev/simple_gpio` |
| 2026-06-30 | `rmmod platform_device_demo` 觸發 kernel oops | 靜態定義的 `platform_device` 沒有提供 `.dev.release`，卸載時 kernel 不知道如何釋放裝置 | 在 `platform_device` 加上 `.dev = { .release = demo_device_release }` |
| 2026-07-28 | `platform_demo.c` 交叉編譯報 `.remove` pointer type 不相容（`int (*)(...)` vs `void (*)(...)`）| kernel API 版本差異：舊版（WSL2 6.6）`platform_driver.remove` 回傳 `int`；新版（Buildroot 6.18）改成 `void`，不能回傳值 | 把 `demo_remove` 簽名從 `static int demo_remove(...)` 改成 `static void demo_remove(...)`，拿掉 `return 0;` |
| 2026-07-28 | 交叉編譯報 `arm-buildroot-linux-gnueabihf-gcc: not found` | PATH 沒有指向 Buildroot 產生的工具鏈 | `export PATH=$HOME/linux-dev/buildroot/output/host/bin:$PATH`，加進 `~/.bashrc` 才會每次自動生效 |
| 2026-07-28 | QEMU 真實環境 `request_irq(80)` 回傳 `-EINVAL` | 手動 `platform_device_register` 塞的 IRQ 號碼沒有透過 device tree 做 irq mapping，sparse IRQ 系統下該號碼沒有對應的 `irq_desc` | 治標：probe 對 IRQ 失敗只警告不 return；治本：要用 Device Tree overlay 讓裝置真的被 `irq_of_parse_and_map()` 解析（W15-16 待補） |
| 2026-07-28 | QEMU 真實環境 `devm_ioremap_resource` 回傳 `-EBUSY`（-16）| `0x10000000` 已被系統其他 driver（推測 v2m_sysreg）佔用記憶體區域 | 換一個目前沒人用的位址（例如 `status="disabled"` 的 `timer@100e4000`）即可成功 |
| 2026-07-27 | `blocking_io` 的 poll 機制：select/poll 等 `EPOLLOUT` 的 process 讀完資料後永遠不會醒 | `blocking_read` 把 `data_ready` 設回 0（buffer 變空、變可寫）後，沒有呼叫 `wake_up_interruptible(&dev->write_wq)`；`poll_wait()` 只是登記，真正喚醒要靠明確呼叫 `wake_up`，不是被動偵測狀態改變 | 在 `blocking_read` 的 `dev->data_ready = 0;` 之後補上 `wake_up_interruptible(&dev->write_wq);`，跟 `blocking_write` 喚醒 `read_wq` 對稱 |

> 註：源碼樹原本屬 root，編譯前先 `sudo chown -R $USER /usr/src/wsl2-headers-$(uname -r)`，之後編模組就不用 sudo（只有 insmod/rmmod 需要 root）。

常見坑速查（來自計劃表）：
- 編譯失敗 → 內核版本不匹配，永遠用 `/lib/modules/$(uname -r)/build`
- device tree 語法錯 → 用 `dtc -I dts -O dtb xxx.dts` 驗證
- compatible 不匹配 → 檢查 `of_device_id` 必須完全相同
- 記憶體洩漏 → 改用 `devm_*` API
- 中斷沒觸發 → `cat /proc/interrupts` 驗證

---

## 🎯 里程碑與履歷時機

| 月份 | 完成度 | 主要成果 | 履歷實力 |
|------|--------|---------|---------|
| 1 | 25% | WSL2 + char driver | ⭐⭐⭐ 實習級 |
| 2 | 50% | LDD3 中段 + platform driver | ⭐⭐⭐⭐ 初級職位 |
| 3 | 62% | LDD3 完成 | ⭐⭐⭐⭐ 可投市場 |
| 4 | 75% | QEMU + GPIO/UART 驅動 | ⭐⭐⭐⭐⭐ |
| 5 | 85% | 多驅動整合 + DT 精通 | ⭐⭐⭐⭐⭐ |
| 6 | 100% | 完整 SoC 平台驅動 + GitHub | ⭐⭐⭐⭐⭐ 資深 |


---

## 🔁 定期檢查（每月末做一次）

- [ ] 本月是否按計劃完成階段任務
- [ ] 代碼質量是否生產級（error handling / cleanup）
- [ ] 筆記是否夠詳細（半年後回頭看得懂）
- [ ] GitHub 是否有更新
- [ ] 有沒有新概念需要深化

---

## 🗓️ 每週開工儀式
1. 看「📍 目前位置」→ 確認下一步
2. 做完一項，把對應 checkbox / 狀態改掉
3. 學到東西寫進「🧠 學習筆記」，出錯寫進「⚠️ 踩坑記錄」
4. 每週至少一個 GitHub commit（再小也提）
