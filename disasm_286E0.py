import struct, capstone
from pathlib import Path

path = Path(r'C:\Users\alpka\Desktop\Game\sro_extensions\output\Release\ext_client.dll')
data = path.read_bytes()
pe_off = struct.unpack('<I', data[0x3C:0x40])[0]
coff_off = pe_off + 4
machine, num_sections, timedate, symtab, numsym, opt_size, chars = struct.unpack('<HHIIIHH', data[coff_off:coff_off+20])
opt_off = coff_off + 20
sec_off = opt_off + opt_size
sections = []
for i in range(num_sections):
    s = data[sec_off + i*40 : sec_off + (i+1)*40]
    name = s[:8].rstrip(b'\x00').decode('ascii', errors='ignore')
    vsize, va, raw_size, raw = struct.unpack('<IIII', s[8:24])
    sections.append((name, va, raw, vsize))

rva = 0x286E0
for name, va, raw, vsize in sections:
    if va <= rva < va + vsize:
        raw_off = raw + (rva - va)
        code = data[raw_off-64:raw_off+64]
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        out = []
        for insn in md.disasm(code, rva-64):
            mark = ' <<< CRASH' if insn.address == rva else ''
            out.append(f'{insn.address:08x}: {insn.bytes.hex():20s} {insn.mnemonic} {insn.op_str}{mark}')
        print('\n'.join(out))
        break
