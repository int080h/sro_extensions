import pefile
import capstone

dll_path = r"C:\Users\alpka\Desktop\Game\sro_extensions\Output\Release\ext_client.dll"
pe = pefile.PE(dll_path)

data = bytes(pe.get_data(0x3b9a0, 0x90))
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
for i in md.disasm(data, 0x3b9a0):
    prefix = "==>" if i.address == 0x3b9e0 else "   "
    print(f"{prefix} 0x{i.address:05x}: {i.mnemonic:8s} {i.op_str}")
