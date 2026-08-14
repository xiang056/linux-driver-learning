# STM32 ↔ Linux Coprocessor Bridge — 設計草稿

> 狀態：草擬中，即將開工。目的：作為轉職 Embedded Linux 的招牌作品，
> 同時展示 STM32 韌體設計能力 + Linux kernel driver 能力。
> 與現有 `linux-dev/` 學習系列的關係：這是應用篇，前提是 blocking_io /
> platform_demo 的 char driver、wait_queue、platform driver 基礎已經打穩。
>
> **學習策略（2026-08-14 調整）**：不先把 Device Tree（W15-16）、sysfs
> （W19-20）、serdev 當成獨立練習學完才開工，而是直接在這個專案的
> Phase 0~2 裡現學現用——DT overlay 綁 Phase 1、sysfs 綁 Phase 2、serdev
> 貫穿全程。原因：抽象練習學完沒有立刻用在真實目標上容易忘，綁定在
> 真實專案裡卡住當場查、當場學，記得住。serdev 的裝置綁定機制本身依賴
> DT（`compatible` 配對觸發 `probe()`），這塊無法跳過，但不需要先做
> 孤立的 DT 練習，直接在 Phase 1 寫這個專案的 overlay 就是在學。

---

## 1. 目標

做一個真實、非模擬的 SoC + MCU coprocessor 系統：

- **STM32**：跑一個簡化版的感測器/致動器韌體（沿用你現有的 ISR 狀態機經驗），
  透過 UART 送出結構化資料，也接受簡單指令。
- **Linux（Raspberry Pi 或同等開發板）**：寫一個 kernel driver，透過
  **`serdev`（serial device）子系統**跟 STM32 對話，而不是單純在 user space
  開 `/dev/ttyUSB0` 讀寫。

**為什麼选 `serdev` 而不是 user space 直接讀 tty：**
`serdev` 是 kernel 專門為「UART 接的 MCU/模組，且需要 kernel 端主動處理」設計的
子系統（實際案例：藍牙 HCI UART controller、u-blox GPS receiver 都是用這套）。
用這個框架而不是 user space read()/write()，代表你懂得「這個場景該用哪個
kernel 子系統」，這是履歷上跟其他人拉開差距的地方——多數自學者不知道
`serdev` 存在，只會寫 user space serial 程式。

---

## 2. 硬體與連線

```
┌─────────────┐   UART (TX/RX/GND)   ┌──────────────────┐
│   STM32     │ ───────────────────▶ │  Raspberry Pi     │
│  (韌體)      │ ◀─────────────────── │  (Linux, serdev   │
│             │                       │   driver)         │
└─────────────┘                       └──────────────────┘
```

- 接線：STM32 UART TX/RX 接到 Pi 的 GPIO14/15（`/dev/ttyAMA0` 或
  `/dev/serial0`），共地。**注意電平**：STM32 通常 3.3V，Pi GPIO 也是
  3.3V，理論上可以直接接，但務必先量測確認，避免燒板子。
- 之後如果要加中斷通知（見第 5 節擴充），STM32 再拉一條 GPIO 給 Pi 當
  IRQ pin。

---

## 3. 通訊協議設計（自訂 frame）

簡單、易 debug 的 framing，之後履歷可以講「自己設計並實作序列協議」：

```
| STX(1B) | LEN(1B) | CMD(1B) | PAYLOAD(0~LEN-1 B) | CRC8(1B) | ETX(1B) |
```

- `STX` = `0xAA`，`ETX` = `0x55`（固定值，方便 kernel 端 resync）
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

## 4. Linux 端 driver 架構

```
serdev_device_driver (probe 綁定 device tree 節點)
        │
        ├─ receive_buf callback：byte-by-byte 餵進 kernel 端的 framing
        │   state machine（跟 STM32 端邏輯對稱，只是換成 kernel C）
        │
        ├─ 收到完整 CMD_SENSOR_REPORT → 更新內部 struct，
        │   透過 sysfs attribute（例如 /sys/class/xxx/sensor_value）
        │   或簡單 char device 讓 user space 讀
        │
        └─ user space write /dev/xxx 或 sysfs attribute
            → driver 組出 CMD_SET_ACTUATOR frame → serdev_device_write()
              送給 STM32
```

**開發階段（建議這樣切，每個階段都能單獨 demo，不用等全部做完）：**

1. **Phase 0 — user space 驗證協議**：先在 Pi 上用一支 Python/C 程式直接開
   `/dev/serial0` 收送 frame，確認 STM32 韌體端的 framing/CRC 邏輯正確。
   這階段完全不碰 kernel，先把「兩邊講同一種語言」這件事做對。
2. **Phase 1 — 最小 serdev driver**：device tree overlay 綁定
   `compatible = "xiang,stm32-bridge"`，`probe()` 成功、`receive_buf`
   印 dmesg 確認有收到 raw bytes，先不解析。
3. **Phase 2 — 加上 framing state machine + sysfs 介面**：驅動內部解析
   完整 frame，`cat /sys/class/.../sensor_value` 能讀到 STM32 送來的資料。
4. **Phase 3 — 反向控制**：`echo 1 > /sys/class/.../actuator` 觸發驅動
   組 frame 送回 STM32，STM32 端真的做出動作（點燈/轉馬達），形成完整
   閉環 demo。
5. **（可選）Phase 4 — 中斷通知**：STM32 資料準備好時拉 GPIO，Pi 端用
   `request_threaded_irq` 處理，取代掉輪詢，呼應你之前 blocking_io 學的
   wait_queue，這裡換成真實硬體中斷版本。

---

## 5. 履歷/面試怎麼講

完成後可以講的故事線（比「做了一個 XX driver」有記憶點很多）：

> 「設計了一套 UART frame 協議，讓 STM32 韌體與 Linux 端透過 kernel 的
> `serdev` 子系統通訊；Linux 端寫 driver 解析協議並透過 sysfs 暴露給
> user space，形成一個完整的 SoC + MCU coprocessor 系統，涵蓋韌體狀態機
> 設計、序列協議與 CRC 校驗、kernel serdev/GPIO IRQ 子系統的實際應用。」

這句話同時證明：韌體能力（你本來就有）+ Linux kernel 子系統選型能力
（新學的，而且選對了子系統）+ 系統整合能力（多數應徵者沒有）。

---

## 6. 待確認/開工前要想清楚的事

- [x] Raspberry Pi 5 4GB 已到手（含散熱片風扇），SD 卡/電源已備妥
- [x] STM32 開發板已有：**STM32F407VG Discovery**（168MHz Cortex-M4），
      沿用自 `Smart-Car` 專案（GitHub: xiang056/Smart-Car）。該專案已有
      **USART6 中斷驅動**收發、byte-by-byte 狀態機解析 BLE(HM-10)/UART
      指令的實戰經驗，跟本專案第 3 節的 framing state machine 是同一套
      邏輯，STM32 端韌體可以直接沿用/改寫既有程式碼，不用從零開始
- [ ] 感測器/致動器要接什麼真實元件？（沒有的話 Phase 3 可以先用板載 LED
      代替致動器，感測資料可以先用 STM32 內建的 ADC 讀電位器代替，之後
      再換真感測器）
- [x] ~~先把 W15-16、W19-20 走完再開工~~ 已改為現學現用（見上方學習策略），
      Device Tree / sysfs / serdev 直接綁在 Phase 0~2 裡學，`serdev` 的
      device tree 綁定跟 platform driver 是同一套邏輯，不是全新的東西

---

<!-- 這是草稿，之後開工前跟 Claude 討論細節時再展開/修改 -->
