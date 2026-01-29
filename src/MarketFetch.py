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
    sock.connect(('localhost', 9999))

    while True:
        data = fetch_price_binance()
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
