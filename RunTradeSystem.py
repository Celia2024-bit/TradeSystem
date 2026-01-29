# -*- coding: utf-8 -*-
import subprocess
import time
import sys
import os

# Configure stdout to handle UTF-8 on Windows
if os.name == 'nt':  # Windows
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8')
    # Set console to UTF-8 mode
    os.system('chcp 65001 > nul 2>&1')

def safe_print(text):
    """Print function that handles Unicode characters safely"""
    try:
        print(text)
    except UnicodeEncodeError:
        # Fallback to ASCII if Unicode fails
        fallback_text = text.encode('ascii', 'replace').decode('ascii')
        print(fallback_text)

def run_script(path, interpreter=None, args=None):
    try:
        cmd = [interpreter, path] if interpreter else [path]
        if args:
            cmd.extend(args)
        subprocess.run(cmd, check=True)
        safe_print(f"✅ Successfully ran: {' '.join(cmd)}")
    except subprocess.CalledProcessError as e:
        safe_print(f"❌ Failed to run: {' '.join(cmd)}\nError: {e}")

def main():
   subprocess.run("rm -rf src/*.cpp.bak", shell=True, check=True)
   subprocess.run("rm -rf src/TradeStrategy/*.cpp.bak", shell=True, check=True)
   run_script("utilLocal/GenerateStrategy/generate_code.py", interpreter="python3")
   #run_script("utilLocal/CppLogInjector.py", interpreter="python3")
   #run_script("tools/Add_check_all.py", interpreter="python3", args=["src"])

   try:
        subprocess.run(["make", "clean"], check=True)
        subprocess.run(["make", "all"], check=True)
        safe_print("✅ Build successful: make all")
   except subprocess.CalledProcessError as e:
        safe_print(f"❌ Build failed\nError: {e}")
        sys.exit(1)

    # Start trading_system in the background first (as server) and capture output
   safe_print("🚀 Starting trading_system as server...")
   with open("result.txt", "w", encoding='utf-8') as f:
        trading_process = subprocess.Popen(
            ["./output/trading_system"], 
            stdout=f, 
            stderr=subprocess.STDOUT
        )

    # Start MarketFetch.py (as client)
   safe_print("🚀 Starting MarketFetch.py as client...")
   market_fetch_process = subprocess.Popen(["python3", "src/MarketFetch.py"])

    # Wait for both processes to complete with a timeout
   TIMEOUT_SECONDS = 42 # A few seconds longer than the main.cppp wait
   start_time = time.time()
   while time.time() - start_time < TIMEOUT_SECONDS:
       if trading_process.poll() is not None and market_fetch_process.poll() is not None:
           break
       time.sleep(1)
       
   # Terminate any remaining processes
   safe_print("🛑 Terminating processes after timeout...")
   if trading_process.poll() is None:
       trading_process.terminate()
   if market_fetch_process.poll() is None:
       market_fetch_process.terminate()
       
   try:
        trading_process.wait(timeout=5)
        safe_print("✅ trading_system completed, output saved to result.txt")
   except subprocess.TimeoutExpired:
        trading_process.kill()
        safe_print("❌ trading_system did not terminate gracefully. Killed.")
   
   try:
        market_fetch_process.wait(timeout=5)
   except subprocess.TimeoutExpired:
        market_fetch_process.kill()
        safe_print("❌ MarketFetch.py did not terminate gracefully. Killed.")

if __name__ == "__main__":
    main()