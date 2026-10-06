import struct
from minidump.minidumpfile import MinidumpFile

dump_path = r"Z:\E\Workspace\Silkroad\RIGID_v234\Dump\4N[2026-10-02 16-12-39]_22_65 AMINIYALIMM_bitwise.dmp"
mf = MinidumpFile.parse(dump_path)

exc_stream = mf.exception.exception_records[0]
rva = exc_stream.ThreadContext.Rva
size = exc_stream.ThreadContext.DataSize

with open(dump_path, "rb") as f:
    f.seek(rva)
    ctx_data = f.read(size)

edi, esi, ebx, edx, ecx, eax, ebp, eip, cs, eflags, esp, ss = struct.unpack('<12I', ctx_data[156:204])

print(f"EIP: 0x{eip:08x}, ESP: 0x{esp:08x}, EBP: 0x{ebp:08x}")
print(f"EAX: 0x{eax:08x}, EBX: 0x{ebx:08x}, ECX: 0x{ecx:08x}, EDX: 0x{edx:08x}, ESI: 0x{esi:08x}, EDI: 0x{edi:08x}")

modules = []
for mod in mf.modules.modules:
    modules.append((mod.baseaddress, mod.size, mod.name))

for base, msize, name in modules:
    if base <= eip < base + msize:
        print(f"\nCrash at {name} + 0x{eip - base:x} (RVA 0x{eip - base:08x})")

print("\n=== CALL STACK ===")
for s in mf.memory_segments_64.memory_segments if mf.memory_segments_64 else mf.memory_segments.memory_segments:
    start = s.start_virtual_address
    end = start + s.size
    if start <= esp < end:
        with open(dump_path, "rb") as f:
            f.seek(s.start_file_address + (esp - start))
            stack_raw = f.read(min(2048, end - esp))
        for offset in range(0, len(stack_raw)-4, 4):
            val = struct.unpack('<I', stack_raw[offset:offset+4])[0]
            for base, msize, name in modules:
                if base <= val < base + msize:
                    mod_name = name.split('\\')[-1]
                    if any(k in mod_name.lower() for k in ['ext_client', 'sro_client']):
                        print(f"  [esp+0x{offset:03x}] 0x{val:08x} -> {mod_name}+0x{val-base:x}")
        break
