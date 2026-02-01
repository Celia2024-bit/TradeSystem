import socket
import time
import json
import random
import csv
import requests
import argparse
import os 
from collections import deque

parser = argparse.ArgumentParser()
parser.add_argument("--output", type=str, default="market_data.csv")
args = parser.parse_args()

MAX_ENTRIES = 2000
FLUSH_INTERVAL = 10
DATA_FILE = args.output

data_queue = deque()
flush_counter = 0


def fetch_price_binance():
    url = "https://api.binance.com/api/v3/ticker/price"
    params = {"symbol": "BTCUSDT"}
    try:
        response = requests.get(url, params=params, timeout=5)
        data = response.json()
        price = float(data["price"])
        timestamp_seconds = time.time()
        return {
            "symbol": "BTC",
            "price": price,
            "timestamp": timestamp_seconds   
        }
    except Exception as e:
        print(f"[ERROR] Binance fetch failed: {e}")
        return None


def fetch_price():
    return {
        "symbol": "BTC",
        "price": round(29500 + random.uniform(-100, 100), 2),
        "timestamp": time.time()
    }

def get_market_data():
    """
    根据环境决定获取数据的方式
    """
    # 检查是否在 GitHub Actions 运行
    is_ci_env = os.environ.get('GITHUB_ACTIONS') == 'true'

    if is_ci_env:
        # 在 CI/CD 环境下直接使用模拟数据，避免网络报错
        print("[INFO] Running in CI/CD environment, using simulated data.")
        return fetch_price()
    else:
        # 在本地环境下，先尝试获取真实币安价格
        data = fetch_price_binance()
        if data:
            return data
        else:
            print("[WARN] Binance API failed, falling back to simulated data.")
            return fetch_price()

def write_csv():
    os.makedirs(os.path.dirname(os.path.abspath(DATA_FILE)), exist_ok=True)
    with open(DATA_FILE, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["symbol", "price", "timestamp"])
        for entry in data_queue:
            if entry:  # Skip None
                writer.writerow([entry["symbol"], entry["price"], entry["timestamp"]])
    print(f"[CSV] Updated with {len(data_queue)} entries")


def start_sender():
    global flush_counter
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # 增加重试机制，防止 C++ Server 还没准备好就 Connect 失败
    connected = False
    while not connected:
        try:
            sock.connect(('localhost', 9999))
            connected = True
        except ConnectionRefusedError:
            print("[WAIT] Waiting for C++ Server to listen on 9999...")
            time.sleep(1)

    while True:
        # 使用包装后的函数替换原来的 fetch_price_binance
        data = get_market_data()
        if data is None:
            print("[SKIP] No valid data to send")
            time.sleep(5)
            continue
        message = json.dumps(data) + "\n"
        sock.sendall(message.encode('utf-8'))

        data_queue.append(data)
        if len(data_queue) > MAX_ENTRIES:
            data_queue.popleft()

        flush_counter += 1
        if flush_counter >= FLUSH_INTERVAL:
            write_csv()
            flush_counter = 0

        time.sleep(1)

if __name__ == "__main__":
    start_sender()
