import glob
import re

files = glob.glob('ext_client/hooks/*.cpp')
for f in files:
    try:
        with open(f, 'r', encoding='utf-8', errors='ignore') as file:
            content = file.read()
            # Find MH_CreateHook or hook creation calls
            mh_calls = re.findall(r'MH_\w+', content)
            if mh_calls:
                print(f"File {f} calls MinHook functions: {set(mh_calls)}")
    except Exception as e:
        print(f"Error reading {f}: {e}")
