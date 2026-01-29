# -*- coding: utf-8 -*-
import subprocess
import time
import sys
import os

def get_config_duration(file_path="config.cfg"):
    try:
        with open(file_path, "r") as f:
            for line in f:
                if line.startswith("RUN_DURATION="):
                    return int(line.split("=")[1].strip())
    except: pass
    return 30

def main():
    run_duration = get_config_duration()
    
    # 1. 启动 C++ Trading System
    print(f"🚀 [1/3] Starting trading_system...")
    with open("result.txt", "w", encoding='utf-8') as f:
        # 注意：这里确保你的 exe 路径正确
        trading_proc = subprocess.Popen(["./output/trading_system"], stdout=f, stderr=subprocess.STDOUT)
    
    cpp_pid = trading_proc.pid # 核心：获取刚启动的 PID
    print(f"✅ trading_system started with PID: {cpp_pid}")

    # 2. 启动监控器 (传入 PID)
    print(f"📊 [2/3] Starting monitor for PID {cpp_pid}...")
    monitor_proc = subprocess.Popen([
        sys.executable, "tools/performance_monitor/run_monitor.py", 
        "--pid", str(cpp_pid),  # 传递 PID
        "--raw", "system_perf_raw.csv"
    ])

    # 3. 启动 MarketFetch
    time.sleep(1) # 给 C++ 一点点启动 Socket 的时间
    print("📈 [3/3] Starting MarketFetch.py...")
    fetch_proc = subprocess.Popen([sys.executable, "src/MarketFetch.py"])

    # 4. 等待 C++ 结束 (它会根据 config.cfg 里的时间自己停)
    try:
        trading_proc.wait() 
        print("✨ trading_system finished execution.")
    except KeyboardInterrupt:
        print("⚠️ Manual stop.")

    # 5. 清理：C++ 停了，其他的也该停了
    for name, p in [("Fetcher", fetch_proc), ("Monitor", monitor_proc)]:
        if p.poll() is None:
            print(f"Stopping {name}...")
            p.terminate()

if __name__ == "__main__":
    main()