import sys
import struct
from minidump.minidumpfile import MinidumpFile

def main():
    if len(sys.argv) < 2:
        print("Usage: python dump_stack.py <dmp_file>")
        return

    dmp_path = sys.argv[1]
    dmp = MinidumpFile.parse(dmp_path)

    if not dmp.exception:
        print("No exception in dump")
        return

    fault_thread_id = dmp.exception.exception_records[0].ThreadId
    print(f"Faulting Thread ID: 0x{fault_thread_id:X}")

    fault_thread = None
    for thread in dmp.threads.threads:
        if thread.ThreadId == fault_thread_id:
            fault_thread = thread
            break

    if not fault_thread:
        print("Could not find faulting thread")
        return

    ctx = fault_thread.ContextObject
    esp = ctx.Esp
    ebp = ctx.Ebp
    eip = ctx.Eip
    print(f"ESP: 0x{esp:08X}, EBP: 0x{ebp:08X}, EIP: 0x{eip:08X}")

    stack_rva = fault_thread.Stack.Rva
    stack_size = fault_thread.Stack.DataSize
    
    with open(dmp_path, "rb") as f:
        f.seek(stack_rva)
        stack_memory = f.read(stack_size)

    stack_start = fault_thread.Stack.StartOfMemoryRange
    stack_end = stack_start + len(stack_memory)

    print(f"Stack memory range: 0x{stack_start:08X} - 0x{stack_end:08X}")

    print("\n--- Stack Call Trace (Only Code Addresses) ---")
    curr = esp
    while curr + 4 <= stack_end:
        offset_in_stack = curr - stack_start
        dword_bytes = stack_memory[offset_in_stack:offset_in_stack+4]
        val = struct.unpack("<I", dword_bytes)[0]
        
        addr_info = get_address_info_str(dmp, val)
        # Check if the address belongs to a code module of interest
        is_interesting = any(x in addr_info for x in ["sro_client", "ext_client", "version.dll"])
        
        if is_interesting:
            print(f"ESP+0x{curr-esp:04X} (0x{curr:08X}) : 0x{val:08X} ({addr_info})")
        curr += 4

def get_address_info_str(dmp, addr):
    if dmp.modules:
        for module in dmp.modules.modules:
            if module.baseaddress <= addr < module.baseaddress + module.size:
                offset = addr - module.baseaddress
                return f"{module.name} + 0x{offset:X}"
    return ""

if __name__ == "__main__":
    main()
