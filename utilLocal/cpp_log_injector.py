#!/usr/bin/env python3
import os
import re
import sys
from pathlib import Path

def find_cpp_files(src_folder):
    """Find all .cpp files in the src folder"""
    cpp_files = []
    src_path = Path(src_folder)
    
    if not src_path.exists():
        print(f"Error: Source folder '{src_folder}' not found")
        return []
    
    for file_path in src_path.rglob("*.cpp"):
        cpp_files.append(file_path)
    
    return cpp_files

def process_cpp_file(file_path):
    """Process a single C++ file and add LOG statements"""
    print(f"Processing: {file_path}")
    
    try:
        with open(file_path, 'r', encoding='utf-8') as file:
            lines = file.readlines()
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return False
    
    modified_lines = []
    i = 0
    modifications_made = 0
    
    while i < len(lines):
        line = lines[i]
        modified_lines.append(line)
        
        # Look for function definitions
        # Pattern to match function signatures (return_type class::function_name(...) or function_name(...))
        function_pattern = r'^\s*(?:[\w\s\*\&<>:,]+\s+)?(?:\w+::)*\w+\s*\([^)]*\)\s*(?:const\s*)?(?:override\s*)?(?:final\s*)?$'
        
        if re.match(function_pattern, line.strip()) and not line.strip().endswith(';'):
            # Check if the next line contains opening brace
            j = i + 1
            found_opening_brace = False
            function_signature = line.strip()
            
            # Look for opening brace within next few lines
            while j < len(lines) and j < i + 5:
                next_line = lines[j].strip()
                if next_line.startswith('{'):
                    found_opening_brace = True
                    break
                elif next_line and not next_line.startswith('//'):
                    # If we find non-comment content, this might be part of function signature
                    function_signature += " " + next_line
                j += 1
            
            if found_opening_brace:
                function_name = __FUNCTION__
                print(f"  Found function: {function_name}")
                
                # Add lines up to and including the opening brace
                while i < j:
                    i += 1
                    if i < len(lines):
                        modified_lines.append(lines[i])
                
                # Add start LOG statement after opening brace
                indent = '    '  # Default indentation
                # Try to detect indentation from next non-empty line
                for k in range(i + 1, min(i + 10, len(lines))):
                    if lines[k].strip():
                        indent = re.match(r'^(\s*)', lines[k]).group(1)
                        break
                
                start_log = f'{indent}LOG(CustomerLogLevel::INFO, "This is start of {function_name}.");\n'
                modified_lines.append(start_log)
                
                # Now find the matching closing brace
                brace_count = 1
                function_body_start = i + 1
                
                while i + 1 < len(lines) and brace_count > 0:
                    i += 1
                    line = lines[i]
                    modified_lines.append(line)
                    
                    # Count braces (simple approach - doesn't handle strings/comments perfectly)
                    for char in line:
                        if char == '{':
                            brace_count += 1
                        elif char == '}':
                            brace_count -= 1
                            if brace_count == 0:
                                # Found the closing brace of the function
                                # Insert stop LOG before this closing brace
                                # Remove the closing brace line we just added
                                modified_lines.pop()
                                
                                # Add stop LOG statement
                                stop_log = f'{indent}LOG(CustomerLogLevel::INFO, "This is stop of {function_name}.");\n'
                                modified_lines.append(stop_log)
                                
                                # Add the closing brace back
                                modified_lines.append(line)
                                modifications_made += 1
                                break
        
        i += 1
    
    # Write modified content back to file
    if modifications_made > 0:
        try:
            with open(file_path, 'w', encoding='utf-8') as file:
                file.writelines(modified_lines)
            print(f"  Added LOG statements to {modifications_made} functions")
            return True
        except Exception as e:
            print(f"Error writing to {file_path}: {e}")
            return False
    else:
        print(f"  No functions found to modify")
        return True

def main():
    src_folder = "src"
    
    # Check if src folder exists
    if not os.path.exists(src_folder):
        print(f"Error: '{src_folder}' folder not found in current directory")
        sys.exit(1)
    
    # Find all .cpp files
    cpp_files = find_cpp_files(src_folder)
    
    if not cpp_files:
        print("No .cpp files found in src folder")
        sys.exit(1)
    
    print(f"Found {len(cpp_files)} C++ files to process")
    
    # Process each file
    success_count = 0
    for cpp_file in cpp_files:
        if process_cpp_file(cpp_file):
            success_count += 1
        print()  # Add blank line between files
    
    print(f"Successfully processed {success_count}/{len(cpp_files)} files")
    
    if success_count == len(cpp_files):
        print("All files processed successfully!")
        sys.exit(0)
    else:
        print("Some files failed to process")
        sys.exit(1)

if __name__ == "__main__":
    main()