import os
import re

workspace = r"c:\Users\alpka\Desktop\Game\sro_ext_client\ext_client"

def check_file(filepath):
    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    # Strip comments to avoid false matches
    # Single-line comments
    content_clean = re.sub(r'//.*', '', content)
    # Multi-line comments
    content_clean = re.sub(r'/\*.*?\*/', '', content_clean, flags=re.DOTALL)
    
    # Find functions with a body. A simple heuristic is a function-like signature followed by '{'
    # excluding class definitions, namespace definitions, structures, enums, etc.
    # Also ignore inline constexpr declarations.
    # Let's count occurrences of { } and see what's inside.
    # Alternatively, we can search for function definitions inside namespaces or classes.
    # Let's look for: auto name(...) -> type { ... } or type name(...) { ... }
    # Let's find patterns like: \b(auto|void|int|bool|float|double|char|std::string|uint32_t|uint8_t|size_t)\s+(\w+)\s*\([^)]*\)\s*(const)?\s*->\s*\w+.*?\{
    # Or just run a simpler check to look for '{' on lines that look like function signatures.
    
    # Let's find lines containing function signatures with '{'
    matches = []
    lines = content_clean.splitlines()
    for idx, line in enumerate(lines):
        # Check if line contains a function definition signature with '{'
        # e.g., "auto func(...) -> ... {" or "void func(...) {" or "func(...) {"
        # exclude namespace, struct, class, enum, union, switch, if, for, while, catch
        stripped = line.strip()
        if '{' in stripped:
            if any(keyword in stripped for keyword in ['namespace', 'struct', 'class', 'enum', 'union', 'switch', 'if ', 'if(', 'for ', 'for(', 'while ', 'while(', 'catch', 'else', '= {', '={']):
                continue
            # Check if it looks like a function definition
            if '(' in stripped and ')' in stripped:
                matches.append((idx + 1, stripped))
    return matches

for root, dirs, files in os.walk(workspace):
    # Skip third-party
    if 'third-party' in root or 'build' in root or '.git' in root:
        continue
    for file in files:
        if file.endswith('.hpp') or file.endswith('.h'):
            filepath = os.path.join(root, file)
            matches = check_file(filepath)
            if matches:
                rel_path = os.path.relpath(filepath, workspace)
                print(f"File: {rel_path} ({len(matches)} matches)")
                for line_num, text in matches[:15]:
                    print(f"  Line {line_num}: {text}")
