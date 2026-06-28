# Linux 驅動學習記憶 (Learning Log)

> 配合計劃表：`C:\Users\love9\Downloads\linux_learning_roadmap.md`
> 目標：嵌入式韌體 → Linux 驅動 / SoC 平台開發，年薪 +20-30%
> 起點：STM32 + embedded C，零 Linux 經驗 ｜ 預計 6-9 個月（4-5 hr/week）

---

## 📍 目前位置（每次開工先看這裡）

- **階段**：第一階段 — Linux 基礎 + 環境建置（8 週）
- **進度**：Week 8 · scull 驅動骨架完成（init/exit/kmalloc/mknod）
- **完成度**：約 25%（hello + hello_param + simple_gpio + ioctl 全部實測通過）
- **環境**：WSL2 Ubuntu 22.04 ｜ 開發目錄 `~/linux-dev/`

### ▶️ 下一步要做的事
1. 實作 `scull` 的 `file_operations`（open / read / write / release）
2. 測試 `/dev/scull0` 讀寫功能
3. 之後加 `lseek` / `ioctl` 擴展

---

## ✅ 進度追蹤總覽

| 階段 | 週次 | 主題 | 狀態 |
|------|------|------|------|
| 一 | W1-2 | WSL2 環境 + 基礎命令 | ✅ 完成 |
| 一 | W3-4 | 內核源碼導航 + 驅動入門 | ✅ 完成 |
| 一 | W5-6 | Character Device Driver | ✅ 完成（simple_gpio 實測通過） |
| 一 | W7-8 | ioctl 擴展 + Ch4 Debugging | ✅ 完成（ioctl 5 個命令實測通過，Ch4 讀完） |
| 二 | W9-10 | lseek + blocking I/O + scull 驅動 | 🟡 進行中（scull 骨架完成） |
| 二 | W11-12 | LDD3 Ch5-6 · 中斷/異步 I/O | ⬜ |
| 二 | W13-14 | LDD3 Ch7-9 · 時間/記憶體/DMA | ⬜ |
| 二 | W15-16 | Platform Driver + Device Tree | ⬜ |
| 三 | W17-18 | QEMU ARM + Buildroot | ⬜ |
| 三 | W19-20 | QEMU 上 GPIO Driver (sysfs) | ⬜ |
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
| scull | `~/linux-dev/scull/` | LDD3 官方 scull + ioctl/lseek 擴展 | 🟡 骨架完成（init/exit/kmalloc），file_operations 待實作 |
| timer | （待建） | 定時器驅動 | ⬜ |
| platform uart | （待建） | platform_driver + device tree | ⬜ |
| gpio_sysfs | （待建） | QEMU GPIO + sysfs 接口 | ⬜ |
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

### Week 5-6
- **2026-06-25** 實測 `simple_gpio.ko` 完整通過
  - **`mknod` 手動建立裝置節點**：`sudo mknod /dev/simple_gpio c <major> 0`；major 號從 `dmesg` 看（本次 240）
  - **寫入需要 root**：`echo "x" > /dev/simple_gpio` 會 Permission denied，要用 `echo "x" | sudo tee /dev/simple_gpio`
  - **踩坑：`simple_gpio` Makefile 的 KDIR 指向 `/lib/modules/.../build`**，該連結不存在；改成指向 `~/linux-dev/my_module/WSL2-Linux-Kernel-linux-msft-wsl-6.6.114.1` 才能編譯（與 `my_module` 相同路徑）

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

- **3 個月**：可投 Linux driver 初級（55-65k）
- **6 個月**：可投 senior embedded（75-90k）

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

> 加油，年薪 +20-30% 就在眼前 🚀
