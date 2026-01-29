# -*- coding: utf-8 -*-
import subprocess
import time
import sys
import os
import shutil
from tools.performance_monitor import constants as C

PYTHON_EXE = "python" if os.name == 'nt' else "python3"
# --- [保留：Windows 编码适配逻辑] ---
if os.name == 'nt':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8')
    os.system('chcp 65001 > nul 2>&1')
    
MONITOR_CONFIG = {
    "interval": 1,        # 监控采样频率 (秒)
    "trend_limit": 2,     # 聚合点数 (每 X 个点生成一个趋势点)
    "result_dir": "build_result"
}

def safe_print(text):
    try:
        print(text)
    except UnicodeEncodeError:
        print(text.encode('ascii', 'replace').decode('ascii'))

def run_script(path, interpreter=None, args=None):
    try:
        cmd = [interpreter, path] if interpreter else [path]
        if args: cmd.extend(args)
        subprocess.run(cmd, check=True)
        safe_print(f"✅ Successfully ran: {' '.join(cmd)}")
    except subprocess.CalledProcessError as e:
        safe_print(f"❌ Failed to run: {' '.join(cmd)}\nError: {e}")

def get_config_duration(file_path="config.cfg"):
    try:
        with open(file_path, "r") as f:
            for line in f:
                if line.startswith("RUN_DURATION="):
                    return int(line.split("=")[1].strip())
    except: pass
    return 30

def main():
    # --- [1. 恢复：环境清理和代码生成] ---
    result_dir = MONITOR_CONFIG["result_dir"]
    if os.path.exists(result_dir):
        safe_print(f"Cleaning old results in {result_dir}...")
        shutil.rmtree(result_dir)  # 删除整个文件夹
    os.makedirs(result_dir)      # 重新创建空的文件夹

    subprocess.run("rm -rf src/*.cpp.bak", shell=True, check=True)
    subprocess.run("rm -rf src/TradeStrategy/*.cpp.bak", shell=True, check=True)
    run_script("utilLocal/GenerateStrategy/generate_code.py", interpreter=PYTHON_EXE)
    # --- [2. 恢复：编译流程] ---
    try:
        subprocess.run(["make", "clean"], check=True)
        subprocess.run(["make", "all"], check=True)
        safe_print("✅ Build successful: make all")
    except subprocess.CalledProcessError as e:
        safe_print(f"❌ Build failed\nError: {e}")
        sys.exit(1)
        
    raw_csv_path = os.path.join(result_dir, C.DEFAULT_RAW_FILE)
    trend_csv_path = os.path.join(result_dir, C.DEFAULT_TREND_FILE)
    result_txt_path = os.path.join(result_dir, "result.txt")

    # --- [3. 启动：Trading System (Server)] ---
    run_duration = get_config_duration()
    safe_print(f"🚀 Starting trading_system (Duration: {run_duration}s)...")
    with open(result_txt_path, "w", encoding='utf-8') as f:
        trading_proc = subprocess.Popen(
            ["./output/trading_system"], 
            stdout=f, 
            stderr=subprocess.STDOUT
        )
    
    cpp_pid = trading_proc.pid
    
    # --- [4. 启动：性能监控 (传入 PID)] ---
    # 这里我们使用你指定的 tools 路径
    monitor_script = os.path.join("tools", "performance_monitor", "run_monitor.py")
    monitor_proc = subprocess.Popen([
        sys.executable, monitor_script, 
        "--pid", str(cpp_pid),
        "--interval", str(MONITOR_CONFIG["interval"]),    # 使用本地变量
        "--limit", str(MONITOR_CONFIG["trend_limit"]),    # 使用本地变量
        "--raw", raw_csv_path,
        "--trend", trend_csv_path
    ])

    # 给 C++ 一点启动 Socket 的时间
    time.sleep(2)

    # --- [5. 启动：MarketFetch (Client)] ---
    safe_print("🚀 Starting MarketFetch.py...")
    # 注意：在 Linux 用 python3, Windows 用 sys.executable
    market_data_path = os.path.join(result_dir, "market_data.csv")
    fetch_proc = subprocess.Popen([sys.executable, "src/MarketFetch.py",
                                   "--output", market_data_path])

    # --- [6. 等待与生命周期管理] ---
    # 逻辑：只要 C++ 还在跑，我们就等着；C++ 一停，我们立刻收割其他进程
    try:
        trading_proc.wait() 
        safe_print("✨ trading_system execution finished.")
    except KeyboardInterrupt:
        safe_print("⚠️ Manual stop requested.")

    # --- [7. 清理：终止所有辅助进程] ---
    safe_print("🛑 Terminating helper processes...")
    for name, p in [("MarketFetcher", fetch_proc), ("Monitor", monitor_proc)]:
        if p.poll() is None:
            p.terminate()
            try: p.wait(timeout=3)
            except: p.kill()

    # --- [8. 绘图：生成报表] ---
    plotter_script = os.path.join("tools", "performance_monitor", "plot_performance.py")
    if os.path.exists(plotter_script):
        safe_print("🎨 Generating reports into build_result...")
        env = os.environ.copy()
        env["PYTHONPATH"] = os.path.dirname(plotter_script)
        
        # 定义图片输出位置
        raw_png = os.path.join(result_dir, "report_raw_detail.png")
        trend_png = os.path.join(result_dir, "report_trend_summary.png")

        subprocess.run([
            PYTHON_EXE, plotter_script,
            "--raw_csv", raw_csv_path,
            "--trend_csv", trend_csv_path,
            "--raw_out", raw_png,
            "--trend_out", trend_png
        ], env=env)

    safe_print(f"🏁 Done. All results are in '{result_dir}/' folder.")
if __name__ == "__main__":
    main()