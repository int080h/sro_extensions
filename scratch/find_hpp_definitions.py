import os
import re

workspace = r"c:\Users\alpka\Desktop\Game\sro_ext_client\ext_client"

def clean_code(content):
    # Strip comments
    content = re.sub(r'//.*', '', content)
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    return content

def find_function_definitions(filepath):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    clean = clean_code(content)
    
    matches = []
    lines = clean.splitlines()
    for idx, line in enumerate(lines):
        s = line.strip()
        if '{' in s:
            if any(k in s for k in ['class ', 'struct ', 'namespace ', 'enum ', 'union ', 'switch ', 'if ', 'if(', 'for ', 'for(', 'while ', 'while(', 'catch', 'else', '= {', '={']):
                continue
            if '(' in s and ')' in s:
                matches.append((idx + 1, s))
            elif ':' in s and ('(' in s or ')' in s):
                matches.append((idx + 1, s))
    return matches

hpp_files = []
for root, dirs, files in os.walk(workspace):
    if 'third-party' in root or 'build' in root or '.git' in root:
        continue
    for file in files:
        if file.endswith('.hpp') or file.endswith('.h'):
            hpp_files.append(os.path.join(root, file))

with open(r"c:\Users\alpka\Desktop\Game\sro_ext_client\scratch\hpp_defs.txt", "w", encoding="utf-8") as out:
    for filepath in hpp_files:
        defs = find_function_definitions(filepath)
        if defs:
            rel = os.path.relpath(filepath, workspace)
            out.write(f"File: {rel}\n")
            for line_no, text in defs:
                out.write(f"  Line {line_no}: {text}\n")
print("Done!")
