# STM32 ↔ Linux Coprocessor Bridge — 設計草稿

> 狀態：草擬中，即將開工。目的：作為轉職 Embedded Linux 的招牌作品，
> 同時展示 STM32 韌體設計能力 + Linux **application/system 層**開發能力。
> 與現有 `linux-dev/` 學習系列的關係：這是應用篇，但主線交付物不再是
> kernel driver，而是一個穩定的 user-space 系統服務。

---

## 0. 方向調整（2026-08-19）

**原本規劃**：主線是寫一個 kernel `serdev` driver（DT overlay + sysfs），
user space 驗證腳本只是 Phase 0 的過渡步驟。

**調整後**：**user-space 系統服務變成主線交付物**，kernel driver 那條路
（DT / serdev / sysfs）保留但降級成「有餘力再做的延伸」，不是完工的必要
條件。

**為什麼改**：目前的 kernel driver 學習還停留在「照著教程走，還沒有能力
獨立判斷」的階段，硬把它當履歷主打作品，做出來的東西自己都不完全理解，
面試被深問容易站不住腳。反過來，Linux **application/system 層開發**
（user-space daemon、systemd 服務、跟硬體/既有 driver 溝通、IPC、log、
穩定性設計）本身就是 Embedded Linux 職缺常見的真實工作內容，不是退而
求其次，而且是你現在的理解程度可以真正「自己講得清楚每一行為什麼這樣寫」
的範圍。kernel driver 相關知識（platform driver、blocking I/O、poll、
DT 概念）繼續當背景累積，面試被問到能對答，但不強迫做出完整成品。

---

## 1. 目標

做一個真實、非模擬的 SoC + MCU coprocessor 系統，**交付物是一個穩定的
Linux 系統服務**，而不是 kernel module：

- **STM32**：跑一個簡化版的感測器/致動器韌體（沿用你現有的 ISR 狀態機
  經驗），透過 UART 送出結構化資料，也接受簡單指令。
- **Linux（Raspberry Pi）**：寫一個 **user-space daemon**，透過
  `/dev/serial0` 跟 STM32 對話，解析協議、處理斷線重連、寫 log，並且
  透過一個簡單介面（Unix domain socket 或本機 HTTP API）讓其他程式能
  查詢/控制 STM32 狀態，用 `systemd` 管理成開機自動啟動的常駐服務。

**為什麼這樣設計能站得住腳（履歷/面試角度）：**
多數自學者做的「串口小程式」是一次性腳本、跑完就結束、斷線不處理、
沒有服務化。你做的是把它當一個**真正會在系統上長期運行的服務**來設計——
斷線自動重連、系統開機自動啟動、有 log 能事後除錯、有介面讓別的程式
使用而不是自己一支腳本孤立運作。這是「寫一支能動的程式」跟「設計一個
可維運的系統」的差距，後者才是 Embedded Linux 職缺實際要用的能力。

---

## 2. 硬體與連線

```
┌─────────────┐   UART (TX/RX/GND)   ┌──────────────────┐
│   STM32     │ ───────────────────▶ │  Raspberry Pi 5   │
│  (韌體)      │ ◀─────────────────── │  (Linux, systemd  │
│             │                       │   daemon)         │
└─────────────┘                       └──────────────────┘
```

- 接線：STM32 UART TX/RX 接到 Pi 的 GPIO14/15（`/dev/ttyAMA0` 或
  `/dev/serial0`），共地。**注意電平**：STM32 通常 3.3V，Pi GPIO 也是
  3.3V，理論上可以直接接，但務必先量測確認，避免燒板子。
- STM32 板：沿用 `Smart-Car` 專案的 **STM32F407VG Discovery**。

---

## 3. 通訊協議設計（自訂 frame）

簡單、易 debug 的 framing，履歷可以講「自己設計並實作序列協議」：

```
| STX(1B) | LEN(1B) | CMD(1B) | PAYLOAD(0~LEN-1 B) | CRC8(1B) | ETX(1B) |
```

- `STX` = `0xAA`，`ETX` = `0x55`（固定值，方便 resync）
- `LEN`：CMD + PAYLOAD 的總長度
- `CMD`：
  - `0x01 CMD_PING` — 心跳/存活確認
  - `0x02 CMD_SENSOR_REPORT`（STM32 → Pi）— payload 是感測資料（例如你熟的
    IMU/心率之類，看你要接什麼感測器）
  - `0x03 CMD_SET_ACTUATOR`（Pi → STM32）— 控制指令（例如 LED、馬達 PWM）
- `CRC8`：整條 frame（不含 STX/ETX）的校驗碼，接法可以先用簡單的 XOR
  版本，之後再升級成真正 CRC8-Maxim，這樣可以講「有考慮資料完整性」

**STM32 端**：狀態機解析 byte-by-byte（你這塊很熟，跟 BLE FTMS 解封包邏輯
類似），收到完整 frame 才處理，維持既有的 ISR 非阻塞風格。

---

## 4. Linux 端架構（主線：user-space 系統服務）

```
stm32bridged（systemd 服務，Python 或 C）
        │
        ├─ 開 /dev/serial0，framing state machine byte-by-byte 解析
        │   （跟 STM32 端邏輯對稱）
        │
        ├─ 斷線偵測 + 自動重連（拔插 USB-UART 轉接板、STM32 重啟都要撐得住）
        │
        ├─ log（systemd journal，關鍵事件：連線/斷線/CRC 錯誤/收到的指令）
        │
        ├─ 收到 CMD_SENSOR_REPORT → 更新內部狀態，透過介面暴露出去
        │   （選一種：Unix domain socket 簡單文字協議 / 本機 HTTP API）
        │
        └─ 外部程式送控制指令 → daemon 組 CMD_SET_ACTUATOR frame →
           寫進 /dev/serial0 送給 STM32
```

**開發階段（每個階段都能單獨 demo，不用等全部做完）：**

1. **Phase 0 — 協議驗證腳本**：Pi 上用 Python 直接開 `/dev/serial0`
   收送 frame，確認 STM32 韌體端的 framing/CRC 邏輯正確。單純一支腳本，
   跑完看結果即可，這階段只求「兩邊講同一種語言」。
2. **Phase 1 — 服務化**：把 Phase 0 的邏輯改寫成長駐程式：斷線重連、
   `try/except` 包好不會一有異常就整支程式掛掉、用 `logging` 模組寫
   log（之後升級成 systemd journal）。
3. **Phase 2 — systemd 整合**：寫 `.service` unit file，設定開機自動
   啟動、崩潰自動重啟（`Restart=on-failure`），`journalctl -u
   stm32bridged` 能看到完整運行紀錄。
4. **Phase 3 — 對外介面**：加一個 Unix domain socket（或簡單 HTTP
   API），讓其他程式能查詢目前 sensor 值、送 actuator 控制指令，
   形成完整閉環 demo（例如寫一支小 CLI 工具 `stm32ctl status` /
   `stm32ctl set-led on`）。
5. **（延伸，非必要）Phase 4 — kernel driver 版本**：等 DT/serdev/
   sysfs 這塊自己覺得真的理解了，可以回頭把同樣的邏輯改寫成 kernel
   `serdev` driver，作為「我也懂 kernel 這條路怎麼做」的延伸展示，
   但不是完工的必要條件，做不到也不影響這個專案的完整性。

---

## 5. 履歷/面試怎麼講

完成後可以講的故事線：

> 「設計了一套 UART frame 協議，讓 STM32 韌體與 Linux 端的一支
> user-space daemon 通訊；daemon 用 systemd 管理，具備斷線自動重連、
> 崩潰自動重啟、journal log，並透過 socket 介面讓其他程式查詢/控制
> STM32 狀態，形成一個可長期運行、可維運的 SoC + MCU coprocessor
> 系統。」

這句話證明：韌體能力（你本來就有）+ Linux 系統服務設計能力（daemon、
systemd、穩定性、可觀測性）+ 協議設計與 CRC 校驗。這些是「東西能動」
跟「東西能穩定跑在真實系統上」的差距，也是多數自學者做不到的地方。

如果之後真的把 Phase 4（kernel driver 版本）也做出來，履歷可以再加一句
「並額外實作了對應的 kernel `serdev` driver 版本，理解 user-space 與
kernel-space 兩種實作路徑的取捨」，變成加分項，但不是主線依賴它。

---

## 6. 待確認/開工前要想清楚的事

- [x] Raspberry Pi 5 4GB 已到手（含散熱片風扇），SD 卡/電源已備妥
- [x] STM32 開發板已有：**STM32F407VG Discovery**（168MHz Cortex-M4），
      沿用自 `Smart-Car` 專案（GitHub: xiang056/Smart-Car）。該專案已有
      **USART6 中斷驅動**收發、byte-by-byte 狀態機解析 BLE(HM-10)/UART
      指令的實戰經驗，跟本專案第 3 節的 framing state machine 是同一套
      邏輯，STM32 端韌體可以直接沿用/改寫既有程式碼，不用從零開始
- [ ] 感測器/致動器要接什麼真實元件？（沒有的話可以先用板載 LED代替
      致動器，感測資料可以先用 STM32 內建的 ADC 讀電位器代替，之後
      再換真感測器）
- [x] Pi 5 燒錄系統進行中，等 microSD 讀卡機到貨（見 LEARNING_LOG）
- [x] kernel driver（DT/serdev/sysfs）降級為延伸項目，主線改為
      user-space systemd 服務，不再是開工前的必要前置知識

---

<!-- 這是草稿，之後開工前跟 Claude 討論細節時再展開/修改 -->
