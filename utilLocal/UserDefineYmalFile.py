#!/usr/bin/env python3
import yaml
import re
import os
import sys
sys.stdout.reconfigure(encoding='utf-8')

def modify_constant(file_path, new_value, constant_name):
    if not os.path.exists(file_path):
        print(f"Error: File {file_path} not found")
        return False
    try:
        with open(file_path, 'r') as file:
            lines = file.readlines()

        modified = False
        for i in range(min(31, len(lines))):
            pattern = rf'constexpr\s+uint32_t\s+{constant_name}\s*=\s*\d+\s*;'
            if re.search(pattern, lines[i]):
                lines[i] = re.sub(
                    rf'(constexpr\s+uint32_t\s+{constant_name}\s*=\s*)\d+(\s*;)',
                    f'\\g<1>{new_value}\\g<2>',
                    lines[i]
                )
                modified = True
                print(f"✅ Modified {constant_name} to {new_value} in {file_path}")
                break

        if not modified:
            print(f"⚠️ Warning: {constant_name} not found in first 31 lines of {file_path}")
            return False

        with open(file_path, 'w') as file:
            file.writelines(lines)

        return True
    except Exception as e:
        print(f"❌ Error modifying {file_path}: {e}")
        return False

def modify_python_variable(file_path, new_value, variable_name):
    """ Modifies a variable in a Python file. """
    if not os.path.exists(file_path):
        print(f"Error: File {file_path} not found")
        return False
    try:
        # Explicitly specify UTF-8 encoding
        with open(file_path, 'r', encoding='utf-8') as file:
            lines = file.readlines()

        modified = False
        for i in range(len(lines)):
            pattern = rf'^\s*{variable_name}\s*=\s*\d+'
            if re.search(pattern, lines[i]):
                lines[i] = re.sub(
                    rf'({variable_name}\s*=\s*)\d+',
                    f'\\g<1>{new_value}',
                    lines[i]
                )
                modified = True
                print(f"✅ Modified {variable_name} to {new_value} in {file_path}")
                break

        if not modified:
            print(f"⚠️ Warning: {variable_name} not found in {file_path}")
            return False

        # Explicitly specify UTF-8 encoding for writing
        with open(file_path, 'w', encoding='utf-8') as file:
            file.writelines(lines)

        return True
    except Exception as e:
        print(f"❌ Error modifying {file_path}: {e}")
        return False

def main():
    config_path = "config/config.yaml"
    if not os.path.exists(config_path):
        print(f"Error: Configuration file '{config_path}' not found")
        sys.exit(1)

    with open(config_path, 'r') as f:
        config = yaml.safe_load(f)

    success = True

    if "tradeTime" in config:
        if not modify_constant('src/main.cpp', config["tradeTime"], 'WAIT_SECONDS'):
            success = False
        timeout_value = config["tradeTime"] + 2
        if not modify_python_variable('RunTradeSystem.py', timeout_value, 'TIMEOUT_SECONDS'):
            success = False
            
    if "maxHistory" in config:
        if not modify_constant('src/StrategyEngine.cpp', config["maxHistory"], 'MAX_HISTORY'):
            success = False       

    if "minHistory" in config:
        if not modify_constant('src/StrategyEngine.cpp', config["minHistory"], 'MIN_HISTORY'):
            success = False

    if success:
        print("✅ All modifications completed successfully")
        sys.exit(0)
    else:
        print("❌ Some modifications failed")
        sys.exit(1)

if __name__ == "__main__":
    main()
