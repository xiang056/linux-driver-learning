# Linux 驅動學習記憶 (Learning Log)

> 配合計劃表：`C:\Users\love9\Downloads\linux_learning_roadmap.md`
> 目標：嵌入式韌體 → Linux 驅動 / SoC 平台開發，年薪 +20-30%
> 起點：STM32 + embedded C，零 Linux 經驗 ｜ 預計 6-9 個月（4-5 hr/week）

---

## 📍 目前位置（每次開工先看這裡）

- **階段**：第一階段 — Linux 基礎 + 環境建置（8 週）
- **進度**：Week 1-2 · WSL2 環境 + Linux 基礎命令
- **完成度**：約 8%（hello.ko + hello_param.ko 完成，含 insmod 實測）
- **環境**：WSL2 Ubuntu 22.04 ｜ 開發目錄 `~/linux-dev/`

### ▶️ 下一步要做的事
1. 讀 LDD3 § 1（導言）+ § 2（構建和運行模塊）
2. 從源碼找到 `printk` 定義位置（小測驗）
3. 開始 `simple_gpio`：第一個字符設備驅動（LDD3 Ch3 簡化版，`register_chrdev` + file_operations）

---

## ✅ 進度追蹤總覽

| 階段 | 週次 | 主題 | 狀態 |
|------|------|------|------|
| 一 | W1-2 | WSL2 環境 + 基礎命令 | 🟡 進行中 |
| 一 | W3-4 | 內核源碼導航 + 驅動入門 | ⬜ |
| 一 | W5-6 | Character Device Driver | ⬜ |
| 一 | W7-8 | 中斷與同步原語 | ⬜ |
| 二 | W9-10 | LDD3 Ch3-4 · scull 驅動 | ⬜ |
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
| simple_gpio | `~/linux-dev/simple_gpio/` | 字符設備驅動（LDD3 Ch3 簡化） | ⬜ 空資料夾 |
| scull | （待建） | LDD3 官方 scull + ioctl/lseek 擴展 | ⬜ |
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

<!-- 之後每週往下加，格式：日期 + 學到的關鍵點 / 踩到的坑 -->

---

## ⚠️ 踩坑記錄（出錯就記，避免重蹈）

| 日期 | 問題 | 原因 | 解法 |
|------|------|------|------|
| 2026-06-11 | `make` 報 `/lib/modules/$(uname -r)/build: No such file` | WSL2 內核更新後，`build` 符號連結掉了 | `sudo ln -sfn /usr/src/wsl2-headers-$(uname -r) /lib/modules/$(uname -r)/build` |
| 2026-06-11 | modpost 報 `_printk`/`module_layout` undefined | 內核源碼樹的 `Module.symvers` 是 0 bytes（只做過 modules_prepare，沒完整編過內核），`CONFIG_MODVERSIONS=y` 解析不到符號 | 在源碼樹 `make -j$(nproc)` 完整編一次內核 → 產生齊全的 Module.symvers（一次性，之後所有模組都能用） |
| 2026-06-11 | `insmod` 預期會報 version magic 不匹配 | 本地重編的 vermagic 結尾多一個 `+`（git 樹 + `CONFIG_LOCALVERSION_AUTO` off 時 `scm_version --short` 會加），運行內核沒有 `+` | build 時設 `LOCALVERSION=`（空但已設定），跳過 setlocalversion 加 `+` 的分支；重生 `kernel.release`/`utsrelease.h` 後重編模組 |

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
