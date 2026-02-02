import logging
import subprocess
import os
import sys

# 确保日志配置生效
logging.basicConfig(
    filename='monitor.log',
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

def initialize_user_process(project_root="./"):
    result = {
        "success": False,
        "message": "",
        "data": {"pids": []}
    }
    
    # 1. 绝对路径转换
    abs_project_root = os.path.abspath(project_root)
    python_exe = sys.executable

    print(f"\n[DEBUG PATH] 项目根目录 (CWD起点): {abs_project_root}")

    try:
        # 2. 验证关键目录是否存在
        required_dirs = ["config", "utilLocal/GenerateStrategy", "src", "output"]
        for d in required_dirs:
            target_path = os.path.join(abs_project_root, d)
            exists = os.path.exists(target_path)
            print(f"[DEBUG PATH] 检查目录 [{d}]: {'✅ 存在' if exists else '❌ 不存在'} -> {target_path}")

        # 3. 执行 Git Pull
        subprocess.run(["git", "pull"], cwd=abs_project_root, check=True, capture_output=True, text=True)
        result["message"] += "✅ Git拉取成功\n"

        # 4. 执行 generate_code.py
        gen_script = os.path.join("utilLocal", "GenerateStrategy", "generate_code.py")
        gen_script_abs = os.path.join(abs_project_root, gen_script)
        
        print(f"[DEBUG PATH] 准备执行脚本: {gen_script_abs}")
        print(f"[DEBUG PATH] 预期配置文件: {os.path.join(abs_project_root, 'config/config.yaml')}")

        # 核心修复：像 RunTradeSystem.py 一样在根目录执行
        gen_res = subprocess.run(
            [python_exe, gen_script], 
            cwd=abs_project_root, 
            capture_output=True, 
            text=True, 
            check=True
        )
        print(f"脚本输出: {gen_res.stdout}")
        result["message"] += "✅ 策略代码生成成功\n"

        # 5. 编译
        subprocess.run(["make", "all"], cwd=abs_project_root, check=True, capture_output=True, text=True)
        result["message"] += "✅ Make编译成功\n"

        # 6. 启动进程
        exe_path = os.path.join(abs_project_root, "output", "trading_system.exe" if os.name == 'nt' else "trading_system")
        market_script = os.path.join(abs_project_root, "src", "MarketFetch.py")

        print(f"[DEBUG PATH] 尝试启动 EXE: {exe_path}")
        print(f"[DEBUG PATH] 尝试启动 PY: {market_script}")

        p_system = subprocess.Popen([exe_path], cwd=abs_project_root)
        result["data"]["pids"].append({"name": "trading_system", "pid": p_system.pid})

        p_market = subprocess.Popen([python_exe, market_script], cwd=abs_project_root)
        result["data"]["pids"].append({"name": "market_fetch", "pid": p_market.pid})
        
        result["success"] = True
        result["message"] += "🚀 系统全组件已启动\n"

    except subprocess.CalledProcessError as e:
        error_info = f"\n--- 错误详情 ---\n命令: {e.cmd}\n路径: {abs_project_root}\nStderr: {e.stderr}\nStdout: {e.stdout}"
        print(error_info)
        result["message"] += error_info
    except Exception as e:
        print(f"❌ 发生异常: {str(e)}")
        result["message"] += str(e)

    return result

if __name__ == "__main__":
    # 执行初始化
    res = initialize_user_process("./")
    print("\n" + "="*50)
    print(f"最终结果: {'SUCCESS' if res['success'] else 'FAILED'}")
    print("="*50)