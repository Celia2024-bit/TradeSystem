# -*- coding: utf-8 -*-
import subprocess
import time
import sys
import os
import shutil
import signal  # Added: for sending SIGINT signals
from tools.performance_monitor import constants as C

PYTHON_EXE = "python" if os.name == 'nt' else "python3"
STOP_FILE_PATH = "./stop"  # Added: stop file path (coordinated with C++ program)
# --- [Keep: Windows encoding adaptation logic] ---
if os.name == 'nt':
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8')
    os.system('chcp 65001 > nul 2>&1')
    
MONITOR_CONFIG = {
    "interval": 1,        # Monitoring sampling frequency (seconds)
    "trend_limit": 2,     # Aggregation points (generate one trend point every X points)
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


# Added: Create stop file to trigger program exit
def create_stop_file():
    """Create stop file to notify C++ program to shut down"""
    try:
        with open(STOP_FILE_PATH, "w", encoding='utf-8') as f:
            f.write("stop")  # Write any content
        safe_print(f"✅ Created stop file: {STOP_FILE_PATH}")
        return True
    except Exception as e:
        safe_print(f"⚠️ Failed to create stop file: {e}")
        return False

# Added: Clean up stop file
def clean_stop_file():
    """Clean up stop file after program exit"""
    if os.path.exists(STOP_FILE_PATH):
        try:
            os.remove(STOP_FILE_PATH)
            safe_print(f"✅ Cleaned stop file: {STOP_FILE_PATH}")
        except Exception as e:
            safe_print(f"⚠️ Failed to clean stop file: {e}")

def main():
    # Pre-cleanup: Delete any residual stop file
    clean_stop_file()
    
    # --- [1. Restore: Environment cleanup and code generation] ---
    result_dir = MONITOR_CONFIG["result_dir"]
    if os.path.exists(result_dir):
        safe_print(f"Cleaning old results in {result_dir}...")
        shutil.rmtree(result_dir)  # Delete entire folder
    os.makedirs(result_dir)      # Recreate empty folder

    subprocess.run("rm -rf src/*.cpp.bak", shell=True, check=True)
    subprocess.run("rm -rf src/TradeStrategy/*.cpp.bak", shell=True, check=True)
    run_script("utilLocal/GenerateStrategy/generate_code.py", interpreter=PYTHON_EXE)
    
    # --- [2. Restore: Build process] ---
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

    # --- [3. Start: Trading System (Server)] ---
    trading_exe = "./output/trading_system.exe" if os.name == 'nt' else "./output/trading_system"
    with open(result_txt_path, "w", encoding='utf-8') as f:
        trading_proc = subprocess.Popen(
            [trading_exe],  # Adapt for Windows .exe suffix
            stdout=f, 
            stderr=subprocess.STDOUT,
            # Added: Create new process group on Windows to receive Ctrl+C
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if os.name == 'nt' else 0
        )
    
    cpp_pid = trading_proc.pid
    
    # --- [4. Start: Performance Monitor (Passing PID)] ---
    monitor_script = os.path.join("tools", "performance_monitor", "run_monitor.py")
    monitor_proc = subprocess.Popen([
        sys.executable, monitor_script, 
        "--pid", str(cpp_pid),
        "--interval", str(MONITOR_CONFIG["interval"]),    # Use local variable
        "--limit", str(MONITOR_CONFIG["trend_limit"]),    # Use local variable
        "--raw", raw_csv_path,
        "--trend", trend_csv_path
    ])

    # Give C++ some time to start the Socket
    time.sleep(2)

    # --- [5. Start: MarketFetch (Client)] ---
    safe_print("🚀 Starting MarketFetch.py...")
    # Note: Use python3 on Linux, sys.executable on Windows
    market_data_path = os.path.join(result_dir, "market_data.csv")
    fetch_proc = subprocess.Popen([sys.executable, "src/MarketFetch.py",
                                   "--output", market_data_path])

    # --- [6. Wait and Lifecycle Management] ---
    run_duration = MONITOR_CONFIG["run_duration"]
    safe_print(f"🚀 Starting trading_system (Duration: {run_duration}s)...")
    total_run_time = run_duration + 5

    safe_print(f"⏳ System running for {total_run_time}s...")
    # Core: Wait for specified runtime (program won't exit automatically, exit triggered at time)
    time.sleep(total_run_time)

    # Time reached, start graceful shutdown process
    safe_print(f"⚠️ Run duration reached! Starting graceful shutdown...")
    if trading_proc.poll() is None:
        # First step: Create stop file to notify program to close itself
        safe_print(f" Creating stop file to notify trading_system...")
        create_stop_file()
        
        # Wait another 3 seconds to see if program recognizes stop file and exits
        time.sleep(3)
        if trading_proc.poll() is None:
            # Stop file ineffective, second step: Force terminate
            safe_print(f"⚠️ Stop file not recognized, force terminating trading_system...")
            trading_proc.terminate() 
            # Ensure it's truly dead
            try: 
                trading_proc.wait(timeout=5)
                safe_print(f"✅ trading_system terminated successfully.")
            except subprocess.TimeoutExpired:
                trading_proc.kill()
                safe_print(f"✅ trading_system killed forcefully.")
    else:
        safe_print(f"✅ trading_system exited after Ctrl+C.")

    # --- [7. Cleanup: Terminate all helper processes] ---
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

    # --- [8. Plotting: Generate reports] ---
    plotter_script = os.path.join("tools", "performance_monitor", "plot_performance.py")
    if os.path.exists(plotter_script):
        safe_print("🎨 Generating reports into build_result...")
        env = os.environ.copy()
        env["PYTHONPATH"] = os.path.dirname(plotter_script)
        
        # Define image output locations
        raw_png = os.path.join(result_dir, "report_raw_detail.png")
        trend_png = os.path.join(result_dir, "report_trend_summary.png")

        subprocess.run([
            PYTHON_EXE, plotter_script,
            "--raw_csv", raw_csv_path,
            "--trend_csv", trend_csv_path,
            "--raw_out", raw_png,
            "--trend_out", trend_png
        ], env=env)

    # Finally clean up stop file
    clean_stop_file()
    
    safe_print(f"🏁 Done. All results are in '{result_dir}/' folder.")

if __name__ == "__main__":
    # Windows requires pywin32 (for sending Ctrl+C)
    if os.name == 'nt':
        try:
            import win32api, win32con, win32process, win32gui
        except ImportError:
            safe_print("⚠️ pywin32 not installed, Ctrl+C simulation will be disabled (use stop file instead).")
            safe_print("   Install with: pip install pywin32")
    main()