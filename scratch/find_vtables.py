import idc

addresses = [0x0102ED74, 0x0106889C]
for addr in addresses:
    print(f"Inspecting address {hex(addr)}:")
    # Print the name at the exact address if any
    name = idc.get_name(addr)
    if name:
        print(f"  Exact name: {name}")
    # Search backwards for the nearest named symbol (which would be the start of the vtable)
    curr = addr
    found = False
    for i in range(100):
        curr_addr = addr - i * 4
        n = idc.get_name(curr_addr)
        if n:
            print(f"  Vtable/Symbol start: {n} at {hex(curr_addr)} (offset +{i*4})")
            found = True
            break
    if not found:
        print("  No named symbol found nearby.")
