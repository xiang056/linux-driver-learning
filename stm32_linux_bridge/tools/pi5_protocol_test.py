#!/usr/bin/env python3
"""Phase 0：驗證 STM32 <-> Pi5 的 framing/CRC 協議（見 DESIGN.md 第 3 節）。

Frame 格式：| STX(1B) | LEN(1B) | CMD(1B) | PAYLOAD(0~LEN-1 B) | CRC8(1B) | ETX(1B) |
- STX=0xAA, ETX=0x55
- LEN = CMD + PAYLOAD 的總長度
- CRC8 = LEN, CMD, PAYLOAD 逐 byte XOR（簡單版，之後可升級成真正 CRC8-Maxim）

跟 STM32 端 Core/Src/protocol.c 的邏輯必須完全對稱。
"""
import argparse
import sys
import time

import serial

STX = 0xAA
ETX = 0x55

CMD_PING = 0x01
CMD_SENSOR_REPORT = 0x02
CMD_SET_ACTUATOR = 0x03


def crc8_xor(data: bytes) -> int:
    crc = 0
    for b in data:
        crc ^= b
    return crc


def build_frame(cmd: int, payload: bytes = b"") -> bytes:
    length = 1 + len(payload)  # CMD + PAYLOAD
    body = bytes([length, cmd]) + payload
    crc = crc8_xor(body)
    return bytes([STX]) + body + bytes([crc, ETX])


def read_frame(ser: serial.Serial, timeout: float) -> tuple[int, bytes] | None:
    """讀一個完整 frame，回傳 (cmd, payload)；逾時或格式錯誤回傳 None。"""
    deadline = time.monotonic() + timeout

    # 找 STX
    while time.monotonic() < deadline:
        b = ser.read(1)
        if not b:
            continue
        if b[0] == STX:
            break
    else:
        return None

    length_b = ser.read(1)
    if not length_b:
        return None
    length = length_b[0]

    body = ser.read(length)
    if len(body) != length:
        return None

    crc_b = ser.read(1)
    etx_b = ser.read(1)
    if not crc_b or not etx_b:
        return None

    if crc8_xor(bytes([length]) + body) != crc_b[0]:
        print(f"  CRC 不符：收到 {crc_b[0]:#04x}，預期 {crc8_xor(bytes([length]) + body):#04x}")
        return None
    if etx_b[0] != ETX:
        print(f"  ETX 不符：收到 {etx_b[0]:#04x}")
        return None

    cmd = body[0]
    payload = body[1:]
    return cmd, payload


def test_ping(ser: serial.Serial, timeout: float) -> bool:
    print("測試 1：CMD_PING")
    ser.write(build_frame(CMD_PING))
    result = read_frame(ser, timeout)
    if result is None:
        print("  失敗：沒收到回應或格式錯誤")
        return False
    cmd, payload = result
    ok = cmd == CMD_PING
    print(f"  {'成功' if ok else '失敗'}：收到 cmd={cmd:#04x} payload={payload!r}")
    return ok


def test_set_actuator(ser: serial.Serial, timeout: float, on: bool) -> bool:
    label = "開燈" if on else "關燈"
    print(f"測試：CMD_SET_ACTUATOR（{label}，PD13 板載橘色 LED）")
    ser.write(build_frame(CMD_SET_ACTUATOR, bytes([1 if on else 0])))
    result = read_frame(ser, timeout)
    if result is None:
        print("  失敗：沒收到回應或格式錯誤")
        return False
    cmd, payload = result
    ok = cmd == CMD_SET_ACTUATOR and payload == bytes([1 if on else 0])
    print(f"  {'成功' if ok else '失敗'}：收到 cmd={cmd:#04x} payload={payload!r}")
    return ok


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/ttyAMA0", help="GPIO14/15 對應的裝置節點（不是 /dev/serial0）")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=1.0)
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=0.05)
    ser.reset_input_buffer()

    results = [
        test_ping(ser, args.timeout),
        test_set_actuator(ser, args.timeout, on=True),
        time.sleep(1) or True,  # 讓你有時間肉眼確認 LED 真的亮了
        test_set_actuator(ser, args.timeout, on=False),
    ]
    results = [r for r in results if isinstance(r, bool)]

    ser.close()

    passed = sum(results)
    print(f"\n共 {len(results)} 項測試，通過 {passed} 項")
    return 0 if passed == len(results) else 1


if __name__ == "__main__":
    sys.exit(main())
