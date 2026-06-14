import json

path = r"C:\Users\alpka\.gemini\antigravity-ide\brain\bfd3b9f7-e1e3-4835-b7c8-f38b92a5c5d1\.system_generated\steps\1207\output.txt"
with open(path, 'r', encoding='utf-8') as f:
    data = json.load(f)

code = data.get('code', '')
for line in code.split('\n'):
    if 'v50' in line or 'v51' in line:
        print(line)
