import sys
from minidump.minidumpfile import MinidumpFile

def main():
    dmp_path = r"4V[2026-06-13 17-48-51]_57_65 AMINIYALIMM_bitwise.dmp"
    mf = MinidumpFile.parse(dmp_path)
    reader = mf.get_reader()
    
    addr = 0x0019f78c
    try:
        data = reader.read(addr, 256)
        print(f"Bytes at {hex(addr)}:")
        print(data)
        # Try to decode as ascii string
        null_idx = data.find(b'\x00')
        if null_idx != -1:
            print("String:", data[:null_idx].decode('ascii', errors='ignore'))
        else:
            print("String (no null):", data.decode('ascii', errors='ignore'))
    except Exception as e:
        print("Error reading address:", e)

if __name__ == "__main__":
    main()
