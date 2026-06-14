import json
import urllib.request

def call_tool(name, arguments):
    req_body = {
        "jsonrpc": "2.0",
        "method": "tools/call",
        "params": {
            "name": name,
            "arguments": arguments
        },
        "id": 1
    }
    req = urllib.request.Request(
        "http://127.0.0.1:13338/mcp",
        data=json.dumps(req_body).encode("utf-8"),
        headers={"Content-Type": "application/json"}
    )
    with urllib.request.urlopen(req) as response:
        return json.loads(response.read().decode("utf-8"))

def main():
    print("Decompiling character selection functions in ISRO client...")
    code = """
import idaapi
import idc

def dec(ea):
    try:
        f = idaapi.get_func(ea)
        if not f:
            return f"No function at {hex(ea)}"
        c = idaapi.decompile(f.start_ea)
        return str(c)
    except Exception as e:
        return f"Decompile error at {hex(ea)}: {e}"

print("=== DECOMPILATION 0xFCDD11 ===")
print(dec(0xFCDD11))
print("=== DECOMPILATION 0xFD69DA ===")
print(dec(0xFD69DA))
"""
    res = call_tool("py_eval", {"code": code})
    structured = res.get("result", {}).get("structuredContent", {})
    print(structured.get("stdout"))

if __name__ == "__main__":
    main()
