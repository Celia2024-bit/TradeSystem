import yaml
import sys
import io
from jinja2 import Environment, FileSystemLoader

sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

# === Read YAML configuration ===
with open('./config/strategy_config.yaml', 'r') as f:
    config = yaml.safe_load(f)

selected_class = config.get('selected_class', '')
obj_name = config.get('obj_name', '')

# Simple validation
if not selected_class or not obj_name:
    raise ValueError("selected_class and obj_name must be provided")

# === Load templates ===
env = Environment(loader=FileSystemLoader('.'))

# Load header template
header_template = env.get_template('utilLocal/GenerateStrategy/strategy_wrapper_header.h.jinja2')
rendered_header = header_template.render(
    selected_class=selected_class,
    obj_name=obj_name
)

# Load implementation template
cpp_template = env.get_template('utilLocal/GenerateStrategy/strategy_wrapper_impl.cpp.jinja2')
rendered_cpp = cpp_template.render(
    selected_class=selected_class,
    obj_name=obj_name
)

# === Write to header file ===
with open('src/StrategyWrapper.h', 'w', encoding='utf-8') as f:
    f.write(rendered_header)

# === Write to CPP file ===
with open('src/StrategyWrapper.cpp', 'w', encoding='utf-8') as f:
    f.write(rendered_cpp)

print(f"✅ Header file generated: StrategyWrapper.h for {selected_class}")
print(f"✅ Implementation file generated: StrategyWrapper.cpp for {selected_class}")


cpp_path = f"src/TradeStrategy/{selected_class}.cpp"
with open('extra_sources.mk', 'w') as f:
    f.write(f"EXTRA_SRCS = {cpp_path}\n")
print(f"✅ Generated EXTRA_SRCS = src/TradeStrategy/{selected_class}.cpp")