import subprocess

def run_script(path, interpreter=None, args=None):
    try:
        cmd = [interpreter, path] if interpreter else [path]
        if args:
            cmd.extend(args)
        subprocess.run(cmd, check=True)
        print(f"✅ Successfully ran: {' '.join(cmd)}")
    except subprocess.CalledProcessError as e:
        print(f"❌ Failed to run: {' '.join(cmd)}\nError: {e}")

def main():
    run_script("utilLocal/UserDefineTradeTimeCountYmalFile.py", interpreter="python3")
    run_script("utilLocal/cpp_log_injector.py", interpreter="python3")  # Assumes it's a compiled binary or shell script
    run_script("tools/Add_check_all.py", interpreter="python3", args=["src"])

    try:
        subprocess.run(["make", "all"], check=True)
        print("✅ Build successful: make all")
    except subprocess.CalledProcessError as e:
        print(f"❌ Build failed\nError: {e}")

if __name__ == "__main__":
    main()
