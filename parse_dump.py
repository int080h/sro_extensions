import sys, struct, os, re

DUMP = r'C:\Users\alpka\Desktop\Game\sro_extensions\sro_ext_client\4N[2026-06-20 23-53-20]_37_65 _.dmp'
LOG = r'C:\Users\alpka\Desktop\Game\sro_extensions\sro_ext_client\dump_analysis_2353.txt'

sys.stdout = open(LOG, 'w', encoding='utf-8')

STREAM_NAMES = {
    0: 'Unused', 1: 'Reserved1', 2: 'Reserved2', 3: 'ThreadList', 4: 'ModuleList',
    5: 'MemoryList', 6: 'Exception', 7: 'SystemInfo', 8: 'ThreadExList',
    9: 'Memory64List', 10: 'CommentA', 11: 'CommentW', 12: 'HandleData',
    13: 'FunctionTable', 14: 'UnloadedModuleList', 15: 'MiscInfo', 16: 'MemoryInfoList',
    17: 'ThreadInfoList', 18: 'HandleOperationList', 19: 'Token', 20: 'JavaScriptData',
    21: 'SystemMemoryInfo', 22: 'ProcessVmCounters', 23: 'IptTrace', 24: 'ThreadNames'
}

def load(path):
    with open(path, 'rb') as f:
        return f.read()

def parse_header(d):
    fmt = '<4sIIIIIQ'
    sig, ver, nstreams, dir_rva, csum, ts, flags = struct.unpack(fmt, d[:32])
    print(f'Header: sig={sig!r} ver_low=0x{ver&0xFFFF:X} nstreams={nstreams} dir_rva=0x{dir_rva:X} checksum=0x{csum:X} ts=0x{ts:X} flags=0x{flags:X}')
    return nstreams, dir_rva

def parse_dir(d, n, dir_rva):
    streams = []
    for i in range(n):
        o = dir_rva + i * 12
        st, ss, srva = struct.unpack('<III', d[o:o+12])
        streams.append((st, ss, srva))
        print(f'  stream {i}: type={st}({STREAM_NAMES.get(st, "?")}) size={ss} rva=0x{srva:X}')
    return streams

def parse_system_info(d, rva, size):
    fmt = '<HHHHIIIIII'
    vals = struct.unpack(fmt, d[rva:rva+struct.calcsize(fmt)])
    arch, level, rev, nproc, ptype, major, minor, build, plat, csd_rva = vals
    ARCH = {0:'x86', 1:'MIPS', 2:'Alpha', 3:'PowerPC', 5:'ARM', 6:'IA64', 9:'x64', 10:'ARM64'}
    print(f'SystemInfo: arch={arch}({ARCH.get(arch, arch)}) level={level} rev={rev} nproc={nproc} os={major}.{minor}.{build} plat={plat} csd_rva=0x{csd_rva:X}')
    return arch

def decode_wide_at(d, rva, max_size=260):
    if rva == 0 or max_size == 0:
        return None
    end = rva
    while end < len(d) - 1 and d[end:end+2] != b'\x00\x00':
        end += 2
    if end == rva:
        return None
    try:
        return d[rva:end+2].decode('utf-16le').rstrip('\x00')
    except Exception:
        return None

def parse_module_list(d, rva, size):
    count = struct.unpack('<I', d[rva:rva+4])[0]
    print(f'ModuleList: count={count}')
    off = rva + 4
    entry = 108
    mods = []
    for i in range(count):
        e = d[off:off+entry]
        base, size_of_img, checksum, tdstamp = struct.unpack('<QIII', e[:20])
        version_info = e[16:68]
        cv_size, cv_rva = struct.unpack('<II', e[68:76])
        misc_size, misc_rva = struct.unpack('<II', e[76:84])
        r0, r1 = struct.unpack('<QQ', e[84:100])
        name = None
        # Try module name from misc or cv record locations; if not, try to decode wide at those offsets.
        for loc_rva, loc_size in ((misc_rva, misc_size), (cv_rva, cv_size)):
            if loc_rva and loc_size:
                s = decode_wide_at(d, loc_rva, loc_size)
                if s:
                    name = s
                    break
        mods.append({
            'i': i, 'base': base, 'size': size_of_img, 'checksum': checksum,
            'tdstamp': tdstamp, 'cv': (cv_rva, cv_size), 'misc': (misc_rva, misc_size),
            'name': name
        })
        off += entry
    print(f'ModuleList fixed end=0x{off:X}, stream end=0x{rva+size:X}')
    # Extract all wide strings from tail as candidate names
    tail = d[off:rva+size]
    names = extract_wide_strings(tail, min_chars=4)
    print('ModuleList tail strings:')
    for s in names:
        print(f'  {s!r}')
    # Assign names if still missing by matching tail strings to module path hints (best effort)
    # For now, just store candidates count.
    return mods

def extract_wide_strings(buf, min_chars=4):
    out = []
    cur = ''
    for i in range(0, len(buf)-1, 2):
        b = buf[i:i+2]
        if b == b'\x00\x00':
            if len(cur) >= min_chars:
                out.append(cur)
            cur = ''
        elif b[1] == 0 and 32 <= b[0] <= 126:
            cur += chr(b[0])
        else:
            cur = ''
    return out

def parse_thread_list(d, rva, size):
    count = struct.unpack('<I', d[rva:rva+4])[0]
    print(f'ThreadList: count={count}')
    off = rva + 4
    entry = 48
    threads = []
    for i in range(count):
        e = d[off:off+entry]
        tid, susp, pclass, prio, teb, stack_start, stack_size, stack_rva, ctx_size, ctx_rva = struct.unpack('<IIIIQ QIIII', e)
        threads.append({
            'tid': tid, 'teb': teb, 'stack_start': stack_start, 'stack_size': stack_size,
            'stack_rva': stack_rva, 'ctx_size': ctx_size, 'ctx_rva': ctx_rva
        })
        print(f'  Thread {tid}: teb=0x{teb:X} stack=0x{stack_start:X}-0x{stack_start+stack_size:X} (rva=0x{stack_rva:X} size={stack_size}) ctx=0x{ctx_rva:X}/{ctx_size}')
        off += entry
    return threads

def parse_exception(d, rva, size):
    tid, _ = struct.unpack('<II', d[rva:rva+8])
    EXC_RECORD_SIZE = 152
    ex = d[rva+8:rva+8+EXC_RECORD_SIZE]
    code, flags, ex_rec, ex_addr, nparams = struct.unpack('<IIQQI', ex[:28])
    params = struct.unpack('<15Q', ex[32:32+120])
    ctx_size, ctx_rva = struct.unpack('<II', d[rva+8+EXC_RECORD_SIZE:rva+8+EXC_RECORD_SIZE+8])
    print(f'Exception: tid={tid} code=0x{code:X}({code}) flags={flags} exc_record=0x{ex_rec:X} addr=0x{ex_addr:X} nparams={nparams} ctx=0x{ctx_rva:X}/{ctx_size}')
    return {'tid': tid, 'code': code, 'addr': ex_addr, 'ctx_size': ctx_size, 'ctx_rva': ctx_rva, 'params': params}

def x86_context_regs(ctx):
    if len(ctx) < 0xC8+4:
        return None
    regs = {}
    regs['edi'] = struct.unpack('<I', ctx[0x9C:0xA0])[0]
    regs['esi'] = struct.unpack('<I', ctx[0xA0:0xA4])[0]
    regs['ebx'] = struct.unpack('<I', ctx[0xA4:0xA8])[0]
    regs['eax'] = struct.unpack('<I', ctx[0xA8:0xAC])[0]
    regs['ecx'] = struct.unpack('<I', ctx[0xAC:0xB0])[0]
    regs['edx'] = struct.unpack('<I', ctx[0xB0:0xB4])[0]
    regs['ebp'] = struct.unpack('<I', ctx[0xB4:0xB8])[0]
    regs['eip'] = struct.unpack('<I', ctx[0xB8:0xBC])[0]
    regs['eflags'] = struct.unpack('<I', ctx[0xBC:0xC0])[0]
    regs['esp'] = struct.unpack('<I', ctx[0xC4:0xC8])[0]
    return regs

def find_module(mods, addr):
    for m in mods:
        if m['base'] <= addr < m['base'] + m['size']:
            return m, addr - m['base']
    return None, None

def module_name(m):
    return (m['name'] or f"mod[{m['i']}]")

def scan_stack(d, mods, rva, size, width):
    if not rva or not size:
        return []
    mem = d[rva:rva+size]
    cands = []
    for i in range(0, len(mem)-width+1, width):
        val = struct.unpack('<I' if width==4 else '<Q', mem[i:i+width])[0]
        if not val:
            continue
        m, off = find_module(mods, val)
        if m:
            cands.append((i, val, m, off))
    return cands

def ebp_chain(d, mods, t, regs):
    if 'ebp' not in regs or 'esp' not in regs:
        return []
    ebp = regs['ebp']
    esp = regs['esp']
    start = t['stack_start']
    size = t['stack_size']
    rva = t['stack_rva']
    if ebp < start or ebp >= start + size:
        return []
    chain = []
    mem = d[rva:rva+size]
    for _ in range(64):
        off = ebp - start
        if off + 8 > size:
            break
        ret = struct.unpack('<I', mem[off+4:off+8])[0]
        m, mo = find_module(mods, ret)
        if m:
            chain.append((ebp, ret, m, mo))
        next_ebp = struct.unpack('<I', mem[off:off+4])[0]
        if next_ebp < start or next_ebp >= start + size or next_ebp <= ebp:
            break
        ebp = next_ebp
    return chain

def main():
    d = load(DUMP)
    print(f'Dump size: {len(d)} bytes')
    n, dir_rva = parse_header(d)
    streams = parse_dir(d, n, dir_rva)
    arch = None
    mods = []
    threads = []
    exc = None
    for st, ss, srva in streams:
        if st == 7:
            arch = parse_system_info(d, srva, ss)
        elif st == 4:
            mods = parse_module_list(d, srva, ss)
        elif st == 3:
            threads = parse_thread_list(d, srva, ss)
        elif st == 6:
            exc = parse_exception(d, srva, ss)
    width = 8 if arch == 9 else 4
    print(f'Address width: {width}')
    print('Modules summary (sorted by base):')
    for m in sorted(mods, key=lambda x: x['base']):
        nm = module_name(m)
        print(f"  [{m['i']:2d}] base=0x{m['base']:08X} size=0x{m['size']:08X} ({m['size']:,}) name={nm!r} tdstamp=0x{m['tdstamp']:08X}")
    # Exception context
    if exc:
        ctx = d[exc['ctx_rva']:exc['ctx_rva']+exc['ctx_size']]
        print(f'Exception context size={len(ctx)}')
        if width == 4:
            regs = x86_context_regs(ctx)
        else:
            regs = {}
            if len(ctx) > 0x100:
                regs['rsp'] = struct.unpack('<Q', ctx[0x98:0xA0])[0]
                regs['rbp'] = struct.unpack('<Q', ctx[0xA0:0xA8])[0]
                regs['rip'] = struct.unpack('<Q', ctx[0xF8:0x100])[0]
        print('Registers:', regs)
        if regs:
            ip = regs['eip'] if width == 4 else regs['rip']
            sp = regs['esp'] if width == 4 else regs['rsp']
            bp = regs.get('ebp') or regs.get('rbp')
            m, off = find_module(mods, ip)
            if m:
                print(f'Exception IP=0x{ip:08X} => {module_name(m)}+0x{off:08X} (base=0x{m["base"]:08X})')
            else:
                print(f'Exception IP=0x{ip:08X} => <unknown module>')
            # Find faulting thread
            for t in threads:
                if t['tid'] == exc['tid']:
                    print(f'Faulting thread: {t}')
                    # EBP chain
                    if width == 4:
                        chain = ebp_chain(d, mods, t, regs)
                        print('EBP chain:')
                        for ebp, ret, mm, mo in chain:
                            print(f'  EBP=0x{ebp:08X} ret=0x{ret:08X} => {module_name(mm)}+0x{mo:08X}')
                    # Scan whole stack for return addresses
                    cands = scan_stack(d, mods, t['stack_rva'], t['stack_size'], width)
                    print(f'Faulting thread stack return addresses (candidates count={len(cands)}):')
                    # Sort by distance from ESP, then print top 40
                    if width == 4:
                        sp_off = sp - t['stack_start']
                    else:
                        sp_off = sp - t['stack_start']
                    cands2 = sorted(cands, key=lambda x: abs(x[0] - sp_off))[:40]
                    for si, val, mm, mo in cands2:
                        print(f'  stack+0x{si:04X} (addr=0x{t["stack_start"]+si:08X}) = 0x{val:08X} => {module_name(mm)}+0x{mo:08X}')
                    break
    # Extract all module-like wide strings from whole dump
    all_names = extract_wide_strings(d, min_chars=6)
    print('Module-like wide strings in dump:')
    seen = set()
    for s in all_names:
        if ('.dll' in s.lower() or '.exe' in s.lower() or '.pyd' in s.lower() or '.sys' in s.lower() or '\\' in s) and s not in seen:
            print(' ', s)
            seen.add(s)

if __name__ == '__main__':
    main()
