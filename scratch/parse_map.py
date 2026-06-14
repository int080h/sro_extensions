import sys
import re

def parse_map(map_path, target_offset):
    print(f"Searching for offset 0x{target_offset:X} in {map_path}")
    
    # We want to find the symbol with the largest address <= target_offset
    best_symbol = None
    best_diff = float('inf')
    
    # Let's read the map file
    with open(map_path, "r", encoding="utf-8", errors="ignore") as f:
        lines = f.readlines()
        
    # Standard MSVC map lines:
    #  0001:00000040       ?func@Class@@...   10001040 f   class.obj
    # Let's extract the Rva+Base or section offset.
    # The third column is the Rva+Base.
    # Wait, the preferred load address is usually at the top of the file:
    # "Preferred load address is 10000000"
    preferred_base = 0x10000000
    for line in lines:
        if "Preferred load address is" in line:
            m = re.search(r"address is ([0-9a-fA-F]+)", line)
            if m:
                preferred_base = int(m.group(1), 16)
                print(f"Preferred load base: 0x{preferred_base:X}")
                break
                
    # Target absolute address in map file:
    target_abs = preferred_base + target_offset
    print(f"Target absolute address: 0x{target_abs:X}")
    
    symbol_pattern = re.compile(r"^\s+([0-9a-fA-F]+):([0-9a-fA-F]+)\s+(\S+)\s+([0-9a-fA-F]+)")
    
    for line in lines:
        m = symbol_pattern.match(line)
        if m:
            sec, off, sym, abs_addr_str = m.groups()
            abs_addr = int(abs_addr_str, 16)
            if abs_addr <= target_abs:
                diff = target_abs - abs_addr
                if diff < best_diff:
                    best_diff = diff
                    best_symbol = (sym, abs_addr, diff)
                    
    if best_symbol:
        sym, addr, diff = best_symbol
        print(f"Closest Symbol: {sym}")
        print(f"Symbol Address: 0x{addr:X}")
        print(f"Difference: +0x{diff:X} bytes")
    else:
        print("No matching symbol found")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python parse_map.py <map_file> <offset_hex>")
    else:
        parse_map(sys.argv[1], int(sys.argv[2], 16))
