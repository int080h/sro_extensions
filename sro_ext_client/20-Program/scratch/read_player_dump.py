import struct
from minidump.minidumpfile import MinidumpFile

dump_path = r"Z:\E\Workspace\Silkroad\RIGID_v234\Dump\4N[2026-10-02 16-12-39]_22_65 AMINIYALIMM_bitwise.dmp"
mf = MinidumpFile.parse(dump_path)
reader = mf.get_reader()

def read_u32(addr):
    try:
        reader.move(addr)
        return struct.unpack('<I', reader.read(4))[0]
    except Exception as e:
        return None

val_1199114 = read_u32(0x01199114)
print(f"dword_1199114 = {hex(val_1199114) if val_1199114 is not None else 'Unreadable'}")

if val_1199114:
    vtable = read_u32(val_1199114)
    cobj = read_u32(val_1199114 + 4)
    print(f"Player vtable = {hex(vtable) if vtable else 'None'}")
    print(f"Player cobj = {hex(cobj) if cobj else 'None'}")
    if cobj:
        cobj_vtable = read_u32(cobj)
        mat = read_u32(cobj + 0xAC)
        print(f"cobj vtable = {hex(cobj_vtable) if cobj_vtable else 'None'}")
        print(f"cobj mat = {hex(mat) if mat else 'None'}")
