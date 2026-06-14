import idc
import idautils
import ida_funcs
import ida_hexrays
import re

funcs = set()

# Get xrefs to sub_B523F0 (sworld instance getter)
xrefs_getter = idautils.XrefsTo(0xB523F0)
for x in xrefs_getter:
    f = ida_funcs.get_func(x.frm)
    if f:
        funcs.add(f.start_ea)

# Get xrefs to unk_13AB480 (sworld global instance)
xrefs_global = idautils.XrefsTo(0x13AB480)
for x in xrefs_global:
    f = ida_funcs.get_func(x.frm)
    if f:
        funcs.add(f.start_ea)

print(f"Total functions found: {len(funcs)}")

offset_accesses = {}

# Get all 51 virtual functions of sworld to add "this"
vtable_addr = 0x1044524
vtable_funcs = [idc.get_wide_dword(vtable_addr + i * 4) for i in range(51)]
for vf in vtable_funcs:
    if vf:
        funcs.add(vf)

for f_ea in sorted(list(funcs)):
    try:
        cfunc = ida_hexrays.decompile(f_ea)
        if not cfunc:
            continue
        lines = str(cfunc).split("\n")
        func_name = ida_funcs.get_func_name(f_ea)
        
        world_vars = set()
        # If it's a vtable function or constructor/destructor of SWorld (names containing B9BDC0 or B9CE30 or starting with sub_B9)
        if f_ea in vtable_funcs or f_ea == 0xB9BDC0 or f_ea == 0xB9CE30:
            world_vars.add("this")
            world_vars.add("a1") # fastcall/thiscall first param
            
        for line in lines:
            if ("sub_B523F0()" in line or "13AB480" in line) and "=" in line:
                parts = line.split("=")
                var_name = parts[0].strip().split()[-1]
                var_name = var_name.replace("*", "").replace("&", "").strip()
                if var_name:
                    world_vars.add(var_name)
        
        if not world_vars:
            continue
            
        for line in lines:
            for wvar in world_vars:
                if wvar in line:
                    # Look for wvar + offset
                    access_matches = re.findall(rf'{wvar}\s*\+\s*(0x[0-9a-fA-F]+|[0-9]+)', line)
                    for match in access_matches:
                        try:
                            if match.startswith("0x"):
                                offset = int(match, 16)
                            else:
                                offset = int(match)
                            if offset not in offset_accesses:
                                offset_accesses[offset] = []
                            offset_accesses[offset].append((func_name, line.strip()))
                        except:
                            pass
                            
                    # Look for wvar[offset]
                    array_matches = re.findall(rf'{wvar}\s*\[\s*(0x[0-9a-fA-F]+|[0-9]+)\s*\]', line)
                    for match in array_matches:
                        try:
                            if match.startswith("0x"):
                                offset = int(match, 16)
                            else:
                                offset = int(match)
                            if offset not in offset_accesses:
                                offset_accesses[offset] = []
                            offset_accesses[offset].append((func_name, line.strip() + " (array index)"))
                        except:
                            pass
                            
    except Exception as e:
        pass

print("\n--- SWORLD FIELD ACCESSES BY OFFSET ---")
for offset in sorted(offset_accesses.keys()):
    print(f"Offset 0x{offset:X} ({offset}):")
    accesses = offset_accesses[offset]
    for fn, line in accesses[:5]:
        print(f"  {fn}: {line}")
    if len(accesses) > 5:
        print(f"  ... and {len(accesses)-5} more accesses")
