import sys
from minidump.minidumpfile import MinidumpFile

dump_path = r"Z:\E\Workspace\Silkroad\RIGID_v234\Dump\4N[2026-10-02 16-12-39]_22_65 AMINIYALIMM_bitwise.dmp"

mf = MinidumpFile.parse(dump_path)

print("=== EXCEPTION LIST ===")
if mf.exception:
    for i, stream in enumerate(mf.exception.exception_records):
        print(f"\n--- Stream {i} ---")
        print(f"ThreadId: {hex(stream.ThreadId)} ({stream.ThreadId})")
        exc = stream.ExceptionRecord
        print(f"ExceptionCode: {exc.ExceptionCode}")
        print(f"ExceptionFlags: {exc.ExceptionFlags}")
        print(f"ExceptionAddress: 0x{exc.ExceptionAddress:08x}")
        print(f"NumberParameters: {exc.NumberParameters}")
        for j, p in enumerate(exc.ExceptionInformation):
            print(f"  Param[{j}]: {p} ({hex(p) if isinstance(p, int) else ''})")
        
        ctx = stream.ThreadContext
        print("ThreadContext:")
        for attr in ['Eip', 'Esp', 'Ebp', 'Eax', 'Ebx', 'Ecx', 'Edx', 'Esi', 'Edi', 'Rip', 'Rsp', 'Rbp', 'Rax', 'Rbx', 'Rcx', 'Rdx', 'Rsi', 'Rdi']:
            if hasattr(ctx, attr):
                print(f"  {attr}: 0x{getattr(ctx, attr):08x}")

print("\n=== MODULES ===")
modules = []
if mf.modules:
    for mod in mf.modules.modules:
        modules.append((mod.baseaddress, mod.size, mod.name))

if mf.exception:
    for stream in mf.exception.exception_records:
        addr = stream.ExceptionRecord.ExceptionAddress
        found = False
        for base, size, name in modules:
            if base <= addr < base + size:
                print(f"\n>>> Crash Address 0x{addr:08x} is in {name} at offset +0x{addr - base:x} (RVA: 0x{addr - base:08x}) <<<")
                found = True
                break
        if not found:
            print(f"\n>>> Crash Address 0x{addr:08x} not in any known module! <<<")

print("\nKey modules:")
for base, size, name in sorted(modules, key=lambda x: x[0]):
    if any(k in name.lower() for k in ['sro_client', 'ext_client', 'd3d', 'engine']):
        print(f"0x{base:08x} - 0x{base+size:08x} : {name}")

# Also inspect stack memory if available
print("\n=== THREADS & STACK ===")
if mf.threads:
    for th in mf.threads.threads:
        if mf.exception and th.ThreadId == mf.exception.exception_records[0].ThreadId:
            print(f"Crashing Thread {hex(th.ThreadId)} Stack: RVA 0x{th.Stack.Memory.Rva:x}, Size {th.Stack.Memory.DataSize}")
            stack_bytes = th.Stack.Memory.read(th.Stack.Memory.Rva, th.Stack.Memory.DataSize, mf.filehandle)
            # Search for return addresses pointing into ext_client or sro_client
            print(f"Stack memory len: {len(stack_bytes)} bytes")
            import struct
            words = []
            for k in range(0, len(stack_bytes) - 4, 4):
                val = struct.unpack('<I', stack_bytes[k:k+4])[0]
                for base, size, name in modules:
                    if any(target in name.lower() for target in ['ext_client', 'sro_client']):
                        if base <= val < base + size:
                            mod_name = name.split('\\')[-1]
                            print(f"  [esp+{k:04x}] 0x{val:08x} ({mod_name} + 0x{val-base:x})")
