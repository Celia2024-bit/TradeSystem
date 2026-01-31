# -*- coding: utf-8 -*-
import subprocess
import time
import sys
import os
import shutil
import signal  # 新增：用于发送SIGINT信号
from tools.performance_monitor import constants as C

PYTHON_EXE = "python" if os.name == 'nt' else "python3"
STOP_FILE_PATH = "./stop"  # 新增：stop文件路径（和C++程序约定）
# --- [保留：Windows 编码适配逻辑] ---
if os.name == 'nt':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8')
    os.system('chcp 65001 > nul 2>&1')
    
MONITOR_CONFIG = {
    "interval": 1,        # 监控采样频率 (秒)
    "trend_limit": 2,     # 聚合点数 (每 X 个点生成一个趋势点)
    "result_dir": "build_result",
    "run_duration": 60
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


# 新增：创建stop文件触发程序退出
def create_stop_file():
    """创建stop文件，提醒C++程序自行关闭"""
    try:
        with open(STOP_FILE_PATH, "w", encoding='utf-8') as f:
            f.write("stop")  # 写入任意内容即可
        safe_print(f"✅ Created stop file: {STOP_FILE_PATH}")
        return True
    except Exception as e:
        safe_print(f"⚠️ Failed to create stop file: {e}")
        return False

# 新增：清理stop文件
def clean_stop_file():
    """程序退出后清理stop文件"""
    if os.path.exists(STOP_FILE_PATH):
        try:
            os.remove(STOP_FILE_PATH)
            safe_print(f"✅ Cleaned stop file: {STOP_FILE_PATH}")
        except Exception as e:
            safe_print(f"⚠️ Failed to clean stop file: {e}")

def main():
    # 前置清理：先删掉残留的stop文件
    clean_stop_file()
    
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
    trading_exe = "./output/trading_system.exe" if os.name == 'nt' else "./output/trading_system"
    with open(result_txt_path, "w", encoding='utf-8') as f:
        trading_proc = subprocess.Popen(
            [trading_exe],  # 适配Windows的.exe后缀
            stdout=f, 
            stderr=subprocess.STDOUT,
            # 新增：Windows下需要创建新的控制台组，才能接收Ctrl+C
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if os.name == 'nt' else 0
        )
    
    cpp_pid = trading_proc.pid
    
    # --- [4. 启动：性能监控 (传入 PID)] ---
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
    run_duration = MONITOR_CONFIG["run_duration"]
    safe_print(f"🚀 Starting trading_system (Duration: {run_duration}s)...")
    total_run_time = run_duration + 5

    safe_print(f"⏳ System running for {total_run_time}s...")
    # 核心：等待指定运行时长（程序不会主动退出，到点就触发退出）
    time.sleep(total_run_time)

    # 到时间了，开始触发优雅退出流程
    safe_print(f"⚠️ Run duration reached! Starting graceful shutdown...")
    if trading_proc.poll() is None:
        #first step: 创建stop文件提醒程序自行关闭
        safe_print(f" Creating stop file to notify trading_system...")
        create_stop_file()
        
        # 再等3秒，看程序是否识别stop文件并退出
        time.sleep(3)
        if trading_proc.poll() is None:
            # stop文件也无效，second step：强制terminate
            safe_print(f"⚠️ Stop file not recognized, force terminating trading_system...")
            trading_proc.terminate() 
            # 确保它真的死了
            try: 
                trading_proc.wait(timeout=5)
                safe_print(f"✅ trading_system terminated successfully.")
            except subprocess.TimeoutExpired:
                trading_proc.kill()
                safe_print(f"✅ trading_system killed forcefully.")
    else:
        safe_print(f"✅ trading_system exited after Ctrl+C.")

    # --- [7. 清理：终止所有辅助进程] ---
    safe_print("🛑 Terminating helper processes...")
    for name, p in [("MarketFetcher", fetch_proc), ("Monitor", monitor_proc)]:
        if p.poll() is None:
            p.terminate()
            try: 
                p.wait(timeout=3)
                safe_print(f"✅ {name} terminated.")
            except subprocess.TimeoutExpired:
                p.kill()
                safe_print(f"✅ {name} killed forcefully.")

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

    # 最后清理stop文件
    clean_stop_file()
    
    safe_print(f"🏁 Done. All results are in '{result_dir}/' folder.")

if __name__ == "__main__":
    # Windows下需要安装pywin32（用于发送Ctrl+C）
    if os.name == 'nt':
        try:
            import win32api, win32con, win32process, win32gui
        except ImportError:
            safe_print("⚠️ pywin32 not installed, Ctrl+C simulation will be disabled (use stop file instead).")
            safe_print("   Install with: pip install pywin32")
    main()