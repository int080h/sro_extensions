import sys
import os

def rva_to_offset(pe_bytes, rva):
    # DOS header
    if pe_bytes[:2] != b'MZ':
        raise ValueError("Not a valid MZ executable")
    
    pe_offset = int.from_bytes(pe_bytes[0x3C:0x40], 'little')
    if pe_bytes[pe_offset:pe_offset+4] != b'PE\x00\x00':
        raise ValueError("Not a valid PE executable")
    
    num_sections = int.from_bytes(pe_bytes[pe_offset+6:pe_offset+8], 'little')
    size_of_optional_header = int.from_bytes(pe_bytes[pe_offset+20:pe_offset+22], 'little')
    
    section_table_offset = pe_offset + 24 + size_of_optional_header
    
    for i in range(num_sections):
        sec_offset = section_table_offset + i * 40
        sec_name = pe_bytes[sec_offset:sec_offset+8].strip(b'\x00').decode('ascii', errors='ignore')
        vsize = int.from_bytes(pe_bytes[sec_offset+8:sec_offset+12], 'little')
        vaddr = int.from_bytes(pe_bytes[sec_offset+12:sec_offset+16], 'little')
        raw_size = int.from_bytes(pe_bytes[sec_offset+16:sec_offset+20], 'little')
        raw_ptr = int.from_bytes(pe_bytes[sec_offset+20:sec_offset+24], 'little')
        
        # Check if RVA falls within this section
        sec_size = max(vsize, raw_size)
        if vaddr <= rva < vaddr + sec_size:
            offset = rva - vaddr + raw_ptr
            return offset, sec_name
            
    return None, None

def main():
    exe_path = r"C:\Users\alpka\Desktop\Game\debug\sro_client.exe"
    if len(sys.argv) > 1:
        exe_path = sys.argv[1]
        
    print(f"Reading {exe_path}...")
    with open(exe_path, "rb") as f:
        data = f.read()
        
    rva = 0x52cc70
    if len(sys.argv) > 2:
        rva = int(sys.argv[2], 16) if sys.argv[2].startswith("0x") else int(sys.argv[2])
        
    offset, sec_name = rva_to_offset(data, rva)
    if offset is None:
        print(f"RVA 0x{rva:x} could not be mapped to any section.")
        return
        
    print(f"RVA 0x{rva:x} maps to section {sec_name} (file offset: 0x{offset:x})")
    
    # Print 32 bytes around the offset
    start_offset = max(0, offset - 16)
    end_offset = min(len(data), offset + 32)
    
    print("\nBytes:")
    chunk = data[start_offset:end_offset]
    for idx, i in enumerate(range(start_offset, end_offset, 16)):
        sub_chunk = data[i : min(end_offset, i + 16)]
        hex_str = " ".join(f"{b:02x}" for b in sub_chunk)
        ascii_str = "".join(chr(b) if 32 <= b < 127 else "." for b in sub_chunk)
        label = f"Offset 0x{i:08x} (RVA 0x{rva - offset + i:08x}):"
        print(f"  {label:<35} {hex_str:<48} |{ascii_str}|")

if __name__ == "__main__":
    main()
