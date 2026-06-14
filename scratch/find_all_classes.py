import os
import re

workspace = r"c:\Users\alpka\Desktop\Game\sro_ext_client\ext_client"

def check_file(filepath):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    # Simple check: find "class \w+ {" or "class \w+ : public \w+ {"
    # ignoring forward declarations
    classes = re.findall(r'class\s+(\w+)\s*(?::\s*[^{]*)?\{', content)
    return classes

for root, dirs, files in os.walk(workspace):
    if 'third-party' in root or 'build' in root or '.git' in root:
        continue
    for file in files:
        if file.endswith('.hpp') or file.endswith('.h'):
            filepath = os.path.join(root, file)
            classes = check_file(filepath)
            if classes:
                rel = os.path.relpath(filepath, workspace)
                print(f"{rel} -> {classes}")
