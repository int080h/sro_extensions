import urllib.request
import json
import re

url = "http://127.0.0.1:13337/output/e915a851-b35d-43b9-86ce-51900c179cdb.json"
try:
    with urllib.request.urlopen(url) as response:
        html = response.read().decode('utf-8')
        data = json.loads(html)
        code = data.get('code', '')
        # Let's search for S_ObjectCreateIndex in the code
        lines = code.split('\n')
        for i, line in enumerate(lines):
            if 'S_ObjectCreateIndex' in line:
                for j in range(max(0, i-5), min(len(lines), i+15)):
                    print(f"{j}: {lines[j]}")
except Exception as e:
    print(f"Error: {e}")
