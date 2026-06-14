import json
import urllib.request
import sys

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
    try:
        req = urllib.request.Request(
            "http://127.0.0.1:13338/mcp",
            data=json.dumps(req_body).encode("utf-8"),
            headers={
                "Content-Type": "application/json",
                "Mcp-Session-Id": "my-session" # bypass session check if required
            }
        )
        with urllib.request.urlopen(req) as response:
            res = json.loads(response.read().decode("utf-8"))
            if "error" in res:
                print("Error:", res["error"])
            else:
                # The MCP result might contain structuredContent or content lists
                result = res.get("result", {})
                if "structuredContent" in result:
                    print(json.dumps(result["structuredContent"], indent=2))
                elif "content" in result:
                    for item in result["content"]:
                        if item.get("type") == "text":
                            print(item.get("text"))
                else:
                    print(json.dumps(result, indent=2))
    except Exception as e:
        print("HTTP Error:", e, file=sys.stderr)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python call_isro.py <tool_name> [args_json]")
        sys.exit(1)
    
    tool_name = sys.argv[1]
    args = {}
    if len(sys.argv) > 2:
        args = json.loads(sys.argv[2])
    
    call_tool(tool_name, args)
