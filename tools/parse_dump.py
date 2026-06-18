import struct
import sys

def main():
    p = sys.argv[1] if len(sys.argv) > 1 else r"C:\Users\alpka\Desktop\Game\sro_extensions\4N[2026-06-18 07-35-02]_51_65 AMINIYALIMM_bitwise.dmp"
    data = open(p, "rb").read()
    print("size", len(data))
    sig, ver, nstreams, dir_rva, cksum, ts, flags = struct.unpack_from("<4sIIIIIQ", data, 0)
    print("sig", sig, "ver", hex(ver), "streams", nstreams, "dir_rva", hex(dir_rva), "ts", ts, "flags", hex(flags))

    names = {0: "ThreadList", 3: "ThreadInfo", 4: "ModuleList", 5: "MemoryList", 6: "Exception", 7: "SystemInfo", 15: "MiscInfo"}

    streams = []
    for i in range(nstreams):
        off = dir_rva + i * 12
        stype, size, rva = struct.unpack_from("<III", data, off)
        if size == 0 and rva == 0:
            continue
        streams.append((stype, size, rva))
        print(f" stream {stype:2} {names.get(stype, '?'):12} size={size:6} rva={hex(rva)}")

    exc_tid = None
    for stype, size, rva in streams:
        if stype == 6:
            tid, align, rec_size = struct.unpack_from("<III", data, rva)
            exc_tid = tid
            print("\n=== EXCEPTION ===")
            print("faulting thread", tid, "rec_size", rec_size)
            code, flags_e, rec, addr, nparams = struct.unpack_from("<IIIII", data, rva + 12)
            exc_names = {
                0xC0000005: "ACCESS_VIOLATION",
                0xC000001D: "ILLEGAL_INSTRUCTION",
                0xC0000094: "INT_DIVIDE_BY_ZERO",
                0xC00000FD: "STACK_OVERFLOW",
                0xE06D7363: "CPP_EH_EXCEPTION",
            }
            print("code", hex(code), exc_names.get(code, ""))
            print("fault IP", hex(addr), "nparams", nparams)
            if nparams:
                params = struct.unpack_from("<" + "Q" * nparams, data, rva + 12 + 20)
                print("params", [hex(x) for x in params])
                if code == 0xC0000005 and nparams >= 2:
                    op = {0: "read", 1: "write", 8: "DEP"}.get(params[0], str(params[0]))
                    print("AV:", op, "at", hex(params[1]))

        if stype == 7:
            proc, major, minor, build, plat, ver, pack, reserved, cpu = struct.unpack_from("<HBBHBBBBQ", data, rva)
            plat_names = {0: "x86", 9: "x64"}
            print("\n=== SYSTEM ===")
            print("platform", plat_names.get(plat, plat), "cpu", hex(cpu))

    modules = []
    for stype, size, rva in streams:
        if stype == 4:
            nmods = struct.unpack_from("<I", data, rva)[0]
            print("\n=== MODULES (%d) ===" % nmods)
            off = rva + 4
            for _ in range(nmods):
                base, msize, checksum, mts, name_rva, mver, sym, symsize, ts2, ts3 = struct.unpack_from(
                    "<IIIIIIIIII", data, off
                )
                slen = struct.unpack_from("<I", data, name_rva)[0]
                name = data[name_rva + 4 : name_rva + 4 + slen].decode("utf-16le", "replace")
                modules.append((base, msize, name))
                print(hex(base), hex(msize), name)
                off += 40

    if modules:
        print("\n=== FAULT MODULE ===")
        for stype, size, rva in streams:
            if stype != 6:
                continue
            _, _, rec_size = struct.unpack_from("<III", data, rva)
            _, _, _, addr, _ = struct.unpack_from("<IIIII", data, rva + 12)
            for base, msize, name in modules:
                if base <= addr < base + msize:
                    print(hex(addr), "in", name, "offset", hex(addr - base))
                    break

    for stype, size, rva in streams:
        if stype == 3:
            n = struct.unpack_from("<I", data, rva)[0]
            print("\n=== THREADS (%d) ===" % n)
            off = rva + 4
            for _ in range(n):
                tid, pad, teb, proc, client, pri, base, start, aff = struct.unpack_from("<IIIIIIII", data, off)
                mark = " <-- FAULT" if exc_tid and tid == exc_tid else ""
                print(" tid", tid, "start", hex(start), mark)
                off += 32

if __name__ == "__main__":
    main()
