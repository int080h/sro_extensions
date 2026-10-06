from minidump.minidumpfile import MinidumpFile

dump_path = r"Z:\E\Workspace\Silkroad\RIGID_v234\Dump\4N[2026-10-02 16-12-39]_22_65 AMINIYALIMM_bitwise.dmp"
mf = MinidumpFile.parse(dump_path)

for stream in mf.exception.exception_records:
    ctx = stream.ThreadContext
    print("Crash Thread Context:")
    for reg in ['Eip', 'Esp', 'Ebp', 'Eax', 'Ebx', 'Ecx', 'Edx', 'Esi', 'Edi', 'EFlags']:
        val = getattr(ctx, reg, None)
        if val is not None:
            print(f"  {reg}: 0x{val:08x}")
