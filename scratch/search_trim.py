import glob
import re

files = glob.glob('ext_client/**/*.cpp', recursive=True) + glob.glob('ext_client/**/*.hpp', recursive=True)

for f in files:
    if "ui_res_catalog" in f:
        continue
    try:
        with open(f, 'r', encoding='utf-8', errors='ignore') as file:
            content = file.read()
            if re.search(r'\bvoid\s+trim\b', content) or re.search(r'\bstd::string\s+trim\b', content):
                print(f"Found trim definition in {f}")
    except Exception as e:
        print(f"Error reading {f}: {e}")
