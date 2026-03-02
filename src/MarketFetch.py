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
    Decide data acquisition method based on environment
    """
    # Check if running in GitHub Actions
    is_ci_env = os.environ.get('GITHUB_ACTIONS') == 'true'

    if is_ci_env:
        # In CI/CD environment, use simulated data directly to avoid network errors
        print("[INFO] Running in CI/CD environment, using simulated data.")
        return fetch_price()
    else:
        # In local environment, try to fetch real Binance price first
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
    # Added retry mechanism to prevent connection failure if C++ Server is not yet ready
    connected = False
    while not connected:
        try:
            sock.connect(('localhost', 9999))
            connected = True
        except ConnectionRefusedError:
            print("[WAIT] Waiting for C++ Server to listen on 9999...")
            time.sleep(1)

    while True:
        # Replace original fetch_price_binance with the wrapped function
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