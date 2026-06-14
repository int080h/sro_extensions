import idautils, idc, ida_funcs

g_d3d_app = 0x013BAE1C
print("Starting search for b_has_focus (0xA3) accesses...")

# Get functions referencing g_d3d_app
functions_to_check = set()
for xref in idautils.XrefsTo(g_d3d_app):
    funcea = ida_funcs.get_func(xref.frm)
    if funcea:
        functions_to_check.add(funcea.start_ea)

for funcea in sorted(list(functions_to_check)):
    f = ida_funcs.get_func(funcea)
    if not f:
        continue
    for ea in idautils.FuncItems(funcea):
        disasm = idc.generate_disasm_line(ea, 0)
        if not disasm:
            continue
        # Check if 0x0A3 or 163 is accessed
        if "0A3h" in disasm or "163" in disasm:
            print(f"Match: {hex(ea)} in {idc.get_func_name(funcea)}: {disasm}")

print("Search completed.")
