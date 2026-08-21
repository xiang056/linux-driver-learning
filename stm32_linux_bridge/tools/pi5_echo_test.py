#!/usr/bin/env python3
"""A-1 echo test：驗證 Pi5 <-> STM32 USART3 基礎通訊（見 Smart-Car/md/RTOS_Practice_Plan.md 4節）。

完成標準：連續 1000 次收發，byte-for-byte 一致、無掉字。
STM32 端邏輯：pi5_link.c 收到什麼就原樣送回同一個 byte。
"""
import argparse
import sys
import time

import serial


def run(port: str, baud: int, count: int, timeout: float) -> int:
    ser = serial.Serial(port, baud, timeout=timeout)
    # 開啟後先清掉可能殘留的舊資料，避免第一輪誤判
    ser.reset_input_buffer()

    ok = 0
    mismatches = []
    no_reply = []

    for i in range(count):
        sent = bytes([i % 256])
        ser.write(sent)
        got = ser.read(1)

        if not got:
            no_reply.append(i)
        elif got != sent:
            mismatches.append((i, sent, got))
        else:
            ok += 1

    ser.close()

    print(f"共送出 {count} 次，成功 {ok}，無回應 {len(no_reply)}，內容不符 {len(mismatches)}")

    if no_reply:
        print(f"無回應的次數（前 10 筆）：{no_reply[:10]}")
    if mismatches:
        print("內容不符（前 10 筆，格式 index/送出/收到）：")
        for idx, sent, got in mismatches[:10]:
            print(f"  #{idx}: sent={sent!r} got={got!r}")

    return 0 if ok == count else 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/ttyAMA0", help="GPIO14/15 對應的裝置節點（不是 /dev/serial0，那個是 debug 接頭）")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--count", type=int, default=1000)
    parser.add_argument("--timeout", type=float, default=0.2, help="單次讀取逾時秒數")
    args = parser.parse_args()

    sys.exit(run(args.port, args.baud, args.count, args.timeout))
