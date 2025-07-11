#!/usr/bin/env python3
import argparse
import re
import os
import sys

def modify_wait_seconds(file_path, new_value, constant_name):
    """
    Modify WAIT_SECONDS in src/main.cpp (lines 0-30)
    """
    if not os.path.exists(file_path):
        print(f"Error: File {file_path} not found")
        return False
    try:
        with open(file_path, 'r') as file:
            lines = file.readlines()
        # Search in lines 0-30 (first 31 lines)
        modified = False
        for i in range(min(31, len(lines))):
            # Pattern to match: constexpr uint32_t WAIT_SECONDS = 300;
            pattern = rf'constexpr\s+uint32_t\s+{constant_name}\s*=\s*\d+\s*;'
            if re.search(pattern, lines[i]):
                # Replace with new value
                lines[i] = re.sub(
                    rf'(constexpr\s+uint32_t\s+{constant_name}\s*=\s*)\d+(\s*;)',
                    f'\\g<1>{new_value}\\g<2>',
                    lines[i]
                )
                modified = True
                print(f"Modified {constant_name} to {new_value} in {file_path}")
                break
        if not modified:
            print(f"Warning: {constant_name} not found in first 31 lines of {file_path}")
            return False
        # Write back to file
        with open(file_path, 'w') as file:
            file.writelines(lines)
        return True
    except Exception as e:
        print(f"Error modifying {file_path}: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description='Modify C++ configuration values')
    parser.add_argument('--tradeTime', type=int, help='Set WAIT_SECONDS value in src/main.cpp')
    parser.add_argument('--tradeCount', type=int, help='Set DATA_COUNT value in src/MarketDataGenerator.cpp')
    args = parser.parse_args()
    
    # Check if at least one parameter is provided
    if args.tradeTime is None and args.tradeCount is None:
        print("Error: At least one parameter (--tradeTime or --tradeCount) must be provided")
        parser.print_help()
        sys.exit(1)
    
    success = True
    
    # Modify WAIT_SECONDS if tradeTime is provided
    if args.tradeTime is not None:
        if not modify_wait_seconds('../src/main.cpp', args.tradeTime, 'WAIT_SECONDS'):
            success = False
    
    # Modify DATA_COUNT if tradeCount is provided
    if args.tradeCount is not None:
        if not modify_wait_seconds('../src/MarketDataGenerator.cpp', args.tradeTime, 'DATA_COUNT'):
            success = False
    
    if success:
        print("All modifications completed successfully")
        sys.exit(0)
    else:
        print("Some modifications failed")
        sys.exit(1)

if __name__ == "__main__":
    main()