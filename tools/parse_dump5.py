import struct
import sys

def main():
    p = sys.argv[1]
    data = open(p, "rb").read()

    # Parse streams
    sig, ver, nstreams, dir_rva, cksum, ts, flags = struct.unpack_from("<4sIIIIIQ", data, 0)
    streams = []
    for i in range(nstreams):
        off = dir_rva + i * 12
        stype, size, rva = struct.unpack_from("<III", data, off)
        streams.append((stype, size, rva))

    # Find exception stream (type 6)
    exc_tid = None
    for stype, size, rva in streams:
        if stype == 6:
            tid = struct.unpack_from('<I', data, rva)[0]
            exc_tid = tid
            code = struct.unpack_from('<I', data, rva + 8)[0]
            addr = struct.unpack_from('<Q', data, rva + 24)[0]
            nparams = struct.unpack_from('<I', data, rva + 32)[0]
            exc_names = {0xC0000005: "ACCESS_VIOLATION", 0x80000003: "BREAKPOINT"}
            print(f"ThreadId={tid} code={hex(code)} {exc_names.get(code, '')} addr={hex(addr)} nparams={nparams}")
            if nparams and nparams <= 15:
                params = struct.unpack_from('<' + 'Q' * nparams, data, rva + 40)
                if code == 0xC0000005 and nparams >= 2:
                    op = {0: 'read', 1: 'write', 8: 'DEP'}.get(params[0], str(params[0]))
                    print(f"AV: {op} at {hex(params[1])}")
            # Thread context
            ctx_size, ctx_rva = struct.unpack_from('<II', data, rva + 160)
            eip = struct.unpack_from('<I', data, ctx_rva + 0xB8)[0]
            esp = struct.unpack_from('<I', data, ctx_rva + 0xC4)[0]
            ebp = struct.unpack_from('<I', data, ctx_rva + 0xB4)[0]
            eax = struct.unpack_from('<I', data, ctx_rva + 0xB0)[0]
            ecx = struct.unpack_from('<I', data, ctx_rva + 0xAC)[0]
            edx = struct.unpack_from('<I', data, ctx_rva + 0xA8)[0]
            ebx = struct.unpack_from('<I', data, ctx_rva + 0xA4)[0]
            esi = struct.unpack_from('<I', data, ctx_rva + 0xA0)[0]
            edi = struct.unpack_from('<I', data, ctx_rva + 0x9C)[0]
            print(f"EIP={hex(eip)} ESP={hex(esp)} EBP={hex(ebp)}")
            print(f"EAX={hex(eax)} ECX={hex(ecx)} EDX={hex(edx)} EBX={hex(ebx)} ESI={hex(esi)} EDI={hex(edi)}")

    # Parse modules (type 4) - MINIDUMP_MODULE is 108 bytes, base is ULONG64
    modules = []
    for stype, size, rva in streams:
        if stype == 4:
            nmods = struct.unpack_from('<I', data, rva)[0]
            off = rva + 4
            for _ in range(nmods):
                if off + 108 > len(data):
                    break
                base, msize, checksum, mts, name_rva = struct.unpack_from('<QIIII', data, off)
                name = "?"
                if name_rva + 4 <= len(data):
                    slen = struct.unpack_from('<I', data, name_rva)[0]
                    if name_rva + 4 + slen <= len(data):
                        name = data[name_rva + 4 : name_rva + 4 + slen].decode("utf-16le", "replace")
                modules.append((base, msize, name))
                off += 108

    # Print relevant modules and find fault
    for base, msize, name in modules:
        if "ext_client" in name.lower() or "silkroad" in name.lower() or name.endswith(".exe"):
            print(f"  {hex(base)} size={hex(msize)} {name}")

    fault_addr = addr & 0xFFFFFFFF
    for base, msize, name in modules:
        if base <= fault_addr < base + msize:
            print(f"Fault: {hex(fault_addr)} in {name}+{hex(fault_addr-base)}")
            break

    # Parse memory ranges (type 5) - MINIDUMP_MEMORY_DESCRIPTOR is 16 bytes (ULONG64 start + 8 byte location)
    ranges = []
    for stype, size, rva in streams:
        if stype == 5:
            n = struct.unpack_from('<I', data, rva)[0]
            off = rva + 4
            for _ in range(n):
                start = struct.unpack_from('<Q', data, off)[0]
                dsize, rva_data = struct.unpack_from('<II', data, off + 8)
                ranges.append((start, dsize, rva_data))
                off += 16

    # Read bytes at EIP
    for start, dsize, rva_data in ranges:
        if start <= eip < start + dsize:
            local_off = rva_data + (eip - start)
            if local_off + 64 <= len(data):
                nearby = data[local_off:local_off+64]
                print(f"\nBYTES AT EIP {hex(eip)}:")
                for i in range(0, len(nearby), 16):
                    print(f"  {eip+i:08x}: {nearby[i:i+16].hex()}")
            break

    # Read stack
    mods_short = [(0x400000, 0x10bd000, 'sro_client'), (0x58400000, 0xe5000, 'ext_client')]
    for base, msize, name in modules:
        if "ext_client" in name.lower():
            mods_short.append((base, msize, 'ext_client'))
        elif name.endswith(".exe"):
            mods_short.append((base, msize, 'sro_client'))
        elif "ntdll" in name.lower():
            mods_short.append((base, msize, 'ntdll'))
        elif "kernel32" in name.lower():
            mods_short.append((base, msize, 'kernel32'))

    for start, dsize, rva_data in ranges:
        if start <= esp < start + dsize:
            local_off = rva_data + (esp - start)
            avail = min(dsize - (esp - start), 512)
            if local_off + avail <= len(data):
                stack = data[local_off:local_off+avail]
                print(f"\nSTACK at ESP {hex(esp)}:")
                for i in range(0, min(len(stack), 320), 4):
                    val = struct.unpack_from('<I', stack, i)[0]
                    mod = ""
                    for mb, ms, mn in mods_short:
                        if mb <= val < mb + ms:
                            mod = f" -> {mn}+{hex(val-mb)}"
                            break
                    print(f"  esp+{i:04x}: {val:08x}{mod}")
            break

if __name__ == "__main__":
    main()
