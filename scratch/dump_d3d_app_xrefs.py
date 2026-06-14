import idautils, idc, ida_funcs

g_d3d_app = 0x013BAE1C
print("Starting xref inspection for g_d3d_app...")

for xref in idautils.XrefsTo(g_d3d_app):
    ea = xref.frm
    disasm = idc.generate_disasm_line(ea, 0)
    func_name = idc.get_func_name(ea)
    print(f"Ref at {hex(ea)} in {func_name}: {disasm}")

print("Completed.")
