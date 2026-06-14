import idautils, idc, ida_funcs

g_d3d_app = 0x013BAE1C
target_offsets = ["0A1h", "0A2h", "0A3h", "161", "162", "163"]

print("Starting optimized offset search in IDA...")
functions_to_check = set()

# Get functions referencing g_d3d_app
for xref in idautils.XrefsTo(g_d3d_app):
    funcea = ida_funcs.get_func(xref.frm)
    if funcea:
        functions_to_check.add(funcea.start_ea)

print(f"Found {len(functions_to_check)} functions referencing g_d3d_app.")

# Scan only these functions
for funcea in sorted(list(functions_to_check)):
    f = ida_funcs.get_func(funcea)
    if not f:
        continue
    for ea in idautils.FuncItems(funcea):
        disasm = idc.generate_disasm_line(ea, 0)
        if not disasm:
            continue
        # Check if any target offset is in the disasm string
        if any(off in disasm for off in target_offsets):
            print(f"Match: {hex(ea)} in {idc.get_func_name(funcea)}: {disasm}")

print("Search completed.")
