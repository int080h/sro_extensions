import struct
import sys

def parse_minidump(filepath):
    print(f"Parsing minidump: {filepath}")
    with open(filepath, "rb") as f:
        data = f.read()

    if len(data) < 32:
        print("File too small to be a minidump")
        return

    signature, version, implementation_version, num_streams, streams_rva, checksum, timestamp, flags = struct.unpack(
        "<4sHHIIIIQ", data[:32]
    )

    if signature != b"MDMP":
        print(f"Invalid signature: {signature}")
        return

    print(f"Signature: MDMP, Version: {version}, Number of Streams: {num_streams}, Directory RVA: {streams_rva}")

    exception_stream = None
    system_info_stream = None
    module_list_stream = None

    # Read directory entries
    for i in range(num_streams):
        off = streams_rva + i * 12
        if off + 12 > len(data):
            break
        stream_type, size, rva = struct.unpack("<III", data[off:off+12])
        # Stream types:
        # 6 = ExceptionStream
        # 11 = SystemInfoStream
        # 9 = ModuleListStream
        if stream_type == 6:
            exception_stream = (rva, size)
        elif stream_type == 11:
            system_info_stream = (rva, size)
        elif stream_type == 9:
            module_list_stream = (rva, size)

    # Parse System Info Stream to check architecture
    is_32bit = True
    if system_info_stream:
        rva, size = system_info_stream
        if rva + 2 <= len(data):
            arch = struct.unpack("<H", data[rva:rva+2])[0]
            # 0: x86, 9: x64
            if arch == 9:
                is_32bit = False
            print(f"Architecture: {'x86 (32-bit)' if is_32bit else 'x64 (64-bit)'}")

    # Parse Exception Stream
    if exception_stream:
        rva, size = exception_stream
        print(f"\n--- Exception Stream (RVA: {rva}, Size: {size}) ---")
        if is_32bit:
            # 32-bit MINIDUMP_EXCEPTION_STREAM
            # ThreadId: 4, Alignment: 4
            # ExceptionRecord:
            #   ExceptionCode: 4
            #   ExceptionFlags: 4
            #   ExceptionRecord: 4
            #   ExceptionAddress: 4
            #   NumberParameters: 4
            #   Alignment: 4
            #   ExceptionInformation: 15 * 4 = 60
            # ThreadContext: Size: 4, Rva: 4
            fmt = "<II" + "IIIII" + "15I" + "II"
            expected_size = 2*4 + 5*4 + 15*4 + 2*4 # 8 + 20 + 60 + 8 = 96 bytes
            if len(data) >= rva + expected_size:
                fields = struct.unpack(fmt, data[rva:rva+expected_size])
                thread_id = fields[0]
                exc_code = fields[2]
                exc_flags = fields[3]
                exc_address = fields[5]
                num_params = fields[6]
                exc_info = fields[7:22]
                context_size = fields[22]
                context_rva = fields[23]

                print(f"Faulting Thread ID: {thread_id}")
                print(f"Exception Code: 0x{exc_code:08X}")
                print(f"Exception Flags: 0x{exc_flags:08X}")
                print(f"Exception Address: 0x{exc_address:08X}")
                print(f"Number of Parameters: {num_params}")
                if num_params > 0:
                    params_str = ", ".join(f"0x{exc_info[j]:08X}" for j in range(min(num_params, 15)))
                    print(f"Exception Parameters: {params_str}")

                # Read thread context (registers)
                # x86 CONTEXT record starts at context_rva
                # ContextFlags is the first 4 bytes.
                # For x86:
                # ContextFlags: 4, Dr0..Dr7: 24, FloatingSave: 112
                # SegGs: 4, SegFs: 4, SegEs: 4, SegDs: 4
                # Edi: 4, Esi: 4, Ebx: 4, Edx: 4, Ecx: 4, Eax: 4
                # Ebp: 4, Eip: 4, SegCs: 4, EFlags: 4, Esp: 4, SegSs: 4
                # ExtendedRegisters: 512
                # Let's unpack the registers.
                # In standard x86 CONTEXT, offset of SegGs is 4 + 24 + 112 = 140 bytes.
                # EDI is at 140 + 16 = 156 bytes.
                # EIP is at 184 bytes.
                if context_rva + 200 <= len(data):
                    ctx_data = data[context_rva:context_rva+200]
                    # Let's extract registers directly by offset:
                    # EDI: 156, ESI: 160, EBX: 164, EDX: 168, ECX: 172, EAX: 176
                    # EBP: 180, EIP: 184, CS: 188, EFlags: 192, ESP: 196, SS: 200
                    regs = struct.unpack("<6I2I4I", ctx_data[156:156+48])
                    edi, esi, ebx, edx, ecx, eax, ebp, eip, cs, eflags, esp, ss = regs
                    print("\n--- Register State ---")
                    print(f"EAX: 0x{eax:08X}   EBX: 0x{ebx:08X}   ECX: 0x{ecx:08X}   EDX: 0x{edx:08X}")
                    print(f"ESI: 0x{esi:08X}   EDI: 0x{edi:08X}   EBP: 0x{ebp:08X}   ESP: 0x{esp:08X}")
                    print(f"EIP: 0x{eip:08X}   EFLAGS: 0x{eflags:08X}")
            else:
                print("Exception stream size mismatch")
        else:
            # 64-bit MINIDUMP_EXCEPTION_STREAM
            # Format differs slightly
            print("64-bit exception parsing not fully implemented in this script")

    # Read modules to see where EIP belongs
    if module_list_stream:
        rva, size = module_list_stream
        if rva + 4 <= len(data):
            num_modules = struct.unpack("<I", data[rva:rva+4])[0]
            print(f"\n--- Modules List ({num_modules} modules) ---")
            # MINIDUMP_MODULE size is 84 bytes in 32-bit/64-bit
            for m in range(num_modules):
                m_off = rva + 4 + m * 84
                if m_off + 84 > len(data):
                    break
                base_addr, mod_size, check, time, name_rva = struct.unpack("<QQIII", data[m_off:m_off+32])
                # Name is stored as a VS_FIXEDFILEINFO followed by a string
                # let's read the string at name_rva
                name = ""
                if name_rva + 4 <= len(data):
                    name_len = struct.unpack("<I", data[name_rva:name_rva+4])[0]
                    if name_rva + 4 + name_len <= len(data):
                        name_bytes = data[name_rva+4:name_rva+4+name_len]
                        name = name_bytes.decode("utf-16le", errors="ignore").rstrip("\x00")
                print(f"0x{base_addr:08X} - 0x{base_addr+mod_size:08X} : {name}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python parse_dmp.py <path_to_dmp>")
    else:
        parse_minidump(sys.argv[1])
