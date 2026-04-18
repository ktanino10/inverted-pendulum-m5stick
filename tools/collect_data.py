#!/usr/bin/env python3
"""
PID チューニングデータ収集スクリプト
シリアルポートからデータを受信し、CSVファイルに保存。
リアルタイムでコマンドも送信可能。

使い方:
  python3 tools/collect_data.py [port]

  デフォルトポート: /dev/cu.usbserial-5B090208431
  データは tools/data/ に保存

操作:
  - Enter: データ保存開始/停止トグル
  - コマンド入力: kp=100, kd=20, on, off, ? 等をそのまま送信
  - q: 終了
"""

import serial
import sys
import os
import time
import threading
from datetime import datetime

# 設定
DEFAULT_PORT = "/dev/cu.usbserial-5B090208431"
BAUD = 115200
DATA_DIR = os.path.join(os.path.dirname(__file__), "data")

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    
    os.makedirs(DATA_DIR, exist_ok=True)
    
    print(f"Connecting to {port}...")
    try:
        ser = serial.Serial(port, BAUD, timeout=0.1)
    except Exception as e:
        print(f"Error: {e}")
        print("USBを接続してください")
        return
    
    time.sleep(2)
    ser.flushInput()
    
    recording = False
    csv_file = None
    data_count = 0
    
    print("=" * 50)
    print("PID Tuning Data Collector")
    print("=" * 50)
    print("Commands:")
    print("  [Enter]       : Start/stop recording")
    print("  kp=100        : Set kp to 100")
    print("  kd=20         : Set kd to 20")
    print("  ki=5          : Set ki to 5")
    print("  po=1.5        : Set Pitch_offset2")
    print("  on / off      : Motor ON/OFF")
    print("  ?             : Show current params")
    print("  q             : Quit")
    print("=" * 50)
    
    # データ受信スレッド
    running = True
    
    def reader():
        nonlocal data_count, csv_file, recording
        while running:
            try:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if not line:
                    continue
                
                # データ行を表示
                if line.startswith("D,"):
                    parts = line.split(",")
                    if len(parts) >= 4:
                        angle = parts[1]
                        power = parts[2]
                        print(f"\r  A={angle:>6s} pw={power:>5s}    ", end="", flush=True)
                    
                    if recording and csv_file:
                        csv_file.write(f"{time.time()},{line}\n")
                        csv_file.flush()
                        data_count += 1
                else:
                    print(f"\n  << {line}")
                    
            except Exception:
                pass
    
    thread = threading.Thread(target=reader, daemon=True)
    thread.start()
    
    try:
        while True:
            cmd = input().strip()
            
            if cmd == "q":
                break
            elif cmd == "":
                # Recording toggle
                recording = not recording
                if recording:
                    fname = datetime.now().strftime("pid_%Y%m%d_%H%M%S.csv")
                    fpath = os.path.join(DATA_DIR, fname)
                    csv_file = open(fpath, "w")
                    csv_file.write("timestamp,Angle,power,powerL,powerR\n")
                    data_count = 0
                    print(f"\n>>> RECORDING to {fname}")
                else:
                    if csv_file:
                        csv_file.close()
                        csv_file = None
                    print(f"\n>>> STOPPED ({data_count} points saved)")
            else:
                # Send command to serial
                ser.write((cmd + "\n").encode())
                print(f"\n  >> Sent: {cmd}")
    
    except KeyboardInterrupt:
        pass
    
    running = False
    if csv_file:
        csv_file.close()
    ser.close()
    print("\nDone.")

if __name__ == "__main__":
    main()
