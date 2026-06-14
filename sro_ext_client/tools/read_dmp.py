#!/usr/bin/env python3
"""
read_dmp.py - Windows minidump crash triage for sro_ext_client.

Usage:
  python read_dmp.py [path.dmp] [--no-color] [--verbose]

With no path, uses the newest .dmp in the project root or current directory.
"""

from __future__ import annotations

import ctypes
import io
import logging
import os
import struct
import sys
from pathlib import Path

logging.getLogger().setLevel(logging.CRITICAL)
logging.disable(logging.ERROR)

try:
    from minidump.minidumpfile import MinidumpFile
    from minidump.streams.ContextStream import CONTEXT, WOW64_CONTEXT
    from minidump.streams.SystemInfoStream import PROCESSOR_ARCHITECTURE
except ImportError:
    print("\033[91mError: 'minidump' library is not installed.\033[0m")
    print("Please run: pip install minidump")
    sys.exit(1)

EXCEPTION_CODES = {
    0x80000002: "STATUS_DATATYPE_MISALIGNMENT",
    0x80000003: "STATUS_BREAKPOINT",
    0x80000004: "STATUS_SINGLE_STEP",
    0xC0000005: "STATUS_ACCESS_VIOLATION",
    0xC0000006: "STATUS_IN_PAGE_ERROR",
    0xC000001D: "STATUS_ILLEGAL_INSTRUCTION",
    0xC0000025: "STATUS_NONCONTINUABLE_EXCEPTION",
    0xC0000094: "STATUS_INTEGER_DIVIDE_BY_ZERO",
    0xC00000FD: "STATUS_STACK_OVERFLOW",
    0xC0000409: "STATUS_STACK_BUFFER_OVERRUN",
    0xC0000374: "STATUS_HEAP_CORRUPTION",
    0xE06D7363: "MSVC_C++_EXCEPTION",
}

# sro_client.exe RVA hints (game code we hook or call into)
SRO_CLIENT_HINTS = [
    (0x00339800, 0x00339A00, "calram_guide_mgr_wnd::refresh_slot_icons (null slot widget / ECX=0 @ +0x8B)"),
    (0x00339A00, 0x00339C00, "calram_guide_mgr_wnd::update_alarm_state neighborhood"),
]

# ext_client.dll RVA hints (Release build, image size ~0xAC000)
EXT_CLIENT_HINTS = [
    (0x0000B700, 0x0000B900, "msvc2008 map_ref::next / is_nil (hash_map tree walk)"),
    (0x0000B600, 0x0000B700, "msvc2008 map/wstring helper (hash_map tree walk or wstring_ref::data)"),
    (0x0000B000, 0x0000B200, "cif_manager::walk_each / walk_map_children"),
    (0x0000AD00, 0x0000AF00, "cif_manager::push_widget"),
    (0x0000BB00, 0x0000BF00, "promo_alarm_hook::apply_from_control / calram_guide_mgr_wnd"),
    (0x0000BE00, 0x0000C000, "calram_guide_mgr_wnd::hide_hidden_slots"),
    (0x00009E00, 0x0000A000, "d3d_hook::render_frame / menu::render"),
    (0x00009F00, 0x0000A200, "menu::render / widget_inspector refresh"),
    (0x0000BD00, 0x0000BE00, "ext_client::update / promo_alarm_hook::apply_from_control"),
    (0x0000BF00, 0x0000C500, "calram_guide_mgr_wnd::hide_promo_widget / promo on_create hooks"),
    (0x0000DF00, 0x0000E000, "clear_static_subobject_texture -> sub_7320B0 (set_texture AV)"),
    (0x0000C300, 0x0000C500, "promo_alarm_hook facebook/magic_lamp guide detour"),
    (0x00002100, 0x00002200, "ext_client::init thread entry"),
    (0x00049600, 0x00049800, "d3d_hook / render_frame"),
    (0x00094100, 0x00094200, "sro_client stream_to_wire (packet hook — ESI register)"),
    (0x0000C800, 0x0000D200, "title_hook::hide_channel_list_button / walk_each"),
    (0x0000D200, 0x0000D800, "title_hook::cif_button_set_visible_detour (removed in fix)"),
]

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent


class Colors:
    HEADER = "\033[95m"
    BLUE = "\033[94m"
    CYAN = "\033[96m"
    GREEN = "\033[92m"
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    END = "\033[0m"
    BOLD = "\033[1m"
    DARK_GRAY = "\033[90m"

    @classmethod
    def disable(cls) -> None:
        for name in ("HEADER", "BLUE", "CYAN", "GREEN", "WARNING", "FAIL", "END", "BOLD", "DARK_GRAY"):
            setattr(cls, name, "")


def find_latest_dmp() -> str | None:
    candidates: list[Path] = []
    for folder in (PROJECT_ROOT, Path.cwd()):
        if not folder.is_dir():
            continue
        candidates.extend(folder.glob("*.dmp"))
        # Bracket/space filenames break naive globs on some setups — scan explicitly.
        try:
            for entry in folder.iterdir():
                if entry.is_file() and entry.suffix.lower() == ".dmp":
                    candidates.append(entry)
        except OSError:
            pass
    if not candidates:
        return None
    unique = {str(p.resolve()): p for p in candidates}
    return str(max(unique.values(), key=lambda p: p.stat().st_mtime))


def resolve_address(mf: MinidumpFile, addr: int) -> tuple[object | None, int]:
    if mf.modules is None:
        return None, 0
    for mod in mf.modules.modules:
        if mod.baseaddress <= addr < mod.endaddress:
            return mod, addr - mod.baseaddress
    return None, 0


def format_module_offset(mf: MinidumpFile, addr: int) -> str:
    mod, offset = resolve_address(mf, addr)
    if mod:
        return f"{os.path.basename(mod.name)}+0x{offset:x}"
    return f"0x{addr:x}"


def explain_access_violation(params: list[int]) -> str:
    if len(params) < 2:
        return "Access violation details unavailable."
    op_type = params[0]
    target_addr = params[1]
    op_str = {0: "read from", 1: "write to", 8: "execute at (DEP)"}.get(op_type, f"op={op_type}")
    msg = f"Attempted to {op_str} 0x{target_addr:x}."
    if target_addr < 0x1000:
        msg += " (near-null pointer)"
    return msg


def module_hint(mod_name: str, rva: int) -> str | None:
    base = os.path.basename(mod_name).lower()
    hints = EXT_CLIENT_HINTS if "ext_client" in base else SRO_CLIENT_HINTS if "sro_client" in base else []
    for lo, hi, text in hints:
        if lo <= rva < hi:
            return text
    return None


def ext_client_hint(rva: int) -> str | None:
    return module_hint("ext_client.dll", rva)


class DbgHelpSymbols:
    """Best-effort PDB symbol lookup via dbghelp.dll (Windows only)."""

    def __init__(self, pdb_dir: Path, module_name: str = "ext_client.dll") -> None:
        self._enabled = False
        self._process = ctypes.c_void_p(-1)
        self._module_base = 0x71850000
        self._module_name = module_name
        if os.name != "nt":
            return
        pdb_path = pdb_dir / module_name
        if not pdb_path.is_file():
            return
        try:
            dbghelp = ctypes.WinDLL("dbghelp")
            dbghelp.SymSetOptions(0x800006)
            dbghelp.SymInitializeW(self._process, None, True)
            dbghelp.SymSetSearchPathW(self._process, str(pdb_dir))
            loaded = dbghelp.SymLoadModuleExW(
                self._process, None, str(pdb_path), None, self._module_base, 0, None, 0
            )
            if loaded:
                self._enabled = True
                self._dbghelp = dbghelp
        except OSError:
            pass

    def rebase(self, actual_base: int) -> None:
        if not self._enabled or actual_base == self._module_base:
            return
        # Re-load at the dump's actual base for correct RVAs.
        try:
            pdb_path = PROJECT_ROOT / "output" / "Release" / self._module_name
            self._dbghelp.SymUnloadModule64(self._process, self._module_base)
            self._module_base = actual_base
            self._dbghelp.SymLoadModuleExW(
                self._process, None, str(pdb_path), None, actual_base, 0, None, 0
            )
        except Exception:
            pass

    def lookup(self, addr: int) -> str | None:
        if not self._enabled:
            return None
        UINT64 = ctypes.c_uint64

        class SYMBOL_INFO(ctypes.Structure):
            _fields_ = [
                ("SizeOfStruct", ctypes.c_uint32),
                ("TypeIndex", ctypes.c_uint32),
                ("Reserved", UINT64 * 2),
                ("Index", ctypes.c_uint32),
                ("Size", ctypes.c_uint32),
                ("ModBase", UINT64),
                ("Flags", ctypes.c_uint32),
                ("Value", UINT64),
                ("Address", UINT64),
                ("Register", ctypes.c_uint32),
                ("Scope", ctypes.c_uint32),
                ("Tag", ctypes.c_uint32),
                ("NameLen", ctypes.c_uint32),
                ("MaxNameLen", ctypes.c_uint32),
                ("Name", ctypes.c_char * 1024),
            ]

        class IMAGEHLP_LINE64(ctypes.Structure):
            _fields_ = [
                ("SizeOfStruct", ctypes.c_uint32),
                ("Key", ctypes.c_void_p),
                ("LineNumber", ctypes.c_uint32),
                ("FileName", ctypes.c_char_p),
                ("Address", UINT64),
            ]

        disp = UINT64(0)
        si = SYMBOL_INFO()
        si.SizeOfStruct = ctypes.sizeof(SYMBOL_INFO)
        si.MaxNameLen = 1023
        if not self._dbghelp.SymFromAddr(self._process, addr, ctypes.byref(disp), ctypes.byref(si)):
            return None
        name = si.Name.decode(errors="replace")
        line = IMAGEHLP_LINE64()
        line.SizeOfStruct = ctypes.sizeof(IMAGEHLP_LINE64)
        line_no = ctypes.c_uint32(0)
        file_line = ""
        if self._dbghelp.SymGetLineFromAddr64(self._process, addr, ctypes.byref(line_no), ctypes.byref(line)):
            file_line = f" @ {line.FileName.decode(errors='replace')}:{line.LineNumber}"
        return f"{name}+0x{disp.value:x}{file_line}"


def parse_context(mf: MinidumpFile, location_desc) -> object:
    mf.file_handle.seek(location_desc.Rva)
    data = mf.file_handle.read(location_desc.DataSize)
    arch = mf.sysinfo.ProcessorArchitecture
    if arch == PROCESSOR_ARCHITECTURE.AMD64:
        return CONTEXT.parse(io.BytesIO(data))
    if arch == PROCESSOR_ARCHITECTURE.INTEL:
        return WOW64_CONTEXT.parse(io.BytesIO(data))
    raise ValueError(f"Unsupported architecture: {arch}")


def read_stack_dword(reader, mem_regions: list, addr: int) -> int | None:
    for region in mem_regions:
        base = region.start
        size = region.size if hasattr(region, "size") else region["size"]
        if base <= addr < base + size:
            off = region.start_file_offset if hasattr(region, "start_file_offset") else region["rva"]
            try:
                data = reader.read(addr, 4)
                return struct.unpack("<I", data)[0]
            except Exception:
                return None
    return None


def walk_ebp_chain(reader, mf, ebp: int, max_frames: int = 24) -> list[int]:
    """Return addresses from EBP-linked stack walk."""
    frames: list[int] = []
    seen_ebp: set[int] = set()
    for _ in range(max_frames):
        if ebp is None or ebp < 0x10000 or ebp in seen_ebp:
            break
        seen_ebp.add(ebp)
        try:
            data = reader.read(ebp, 8)
        except Exception:
            break
        saved_ebp, ret_addr = struct.unpack("<II", data)
        if ret_addr < 0x10000:
            break
        mod, _ = resolve_address(mf, ret_addr)
        if mod:
            frames.append(ret_addr)
        if saved_ebp is None or saved_ebp <= ebp:
            break
        ebp = saved_ebp
    return frames


def scan_stack_code_pointers(reader, mf, esp: int, limit: int, is_64: bool) -> list[int]:
    ptr_size = 8 if is_64 else 4
    seen: set[int] = set()
    ordered: list[int] = []
    stack_data = b""
    for chunk_off in range(0, limit, 256):
        try:
            stack_data += reader.read(esp + chunk_off, min(256, limit - chunk_off))
        except Exception:
            break
    if not stack_data:
        return ordered
    for offset in range(0, len(stack_data) - ptr_size + 1, ptr_size):
        ptr_val = int.from_bytes(stack_data[offset : offset + ptr_size], "little")
        if ptr_val in seen or ptr_val < 0x10000:
            continue
        mod, _ = resolve_address(mf, ptr_val)
        if mod:
            seen.add(ptr_val)
            ordered.append(ptr_val)
    return ordered


def frame_line(mf: MinidumpFile, addr: int, symbols: DbgHelpSymbols, prefix: str = "  ") -> str:
    mod, rva = resolve_address(mf, addr)
    loc = format_module_offset(mf, addr)
    mod_base = os.path.basename(mod.name).lower() if mod else ""
    sym = symbols.lookup(addr) if mod and "ext_client" in mod_base else None
    hint = ""
    if mod:
        hint_text = module_hint(mod.name, rva)
        if hint_text:
            hint = f"  {Colors.DARK_GRAY}// {hint_text}{Colors.END}"
    sym_part = f"  {Colors.CYAN}{sym}{Colors.END}" if sym else ""
    return f"{prefix}{Colors.WARNING}0x{addr:08x}{Colors.END}  {Colors.GREEN}{loc}{Colors.END}{sym_part}{hint}"


def print_quick_diagnosis(
    mf: MinidumpFile,
    reader,
    ex_rec,
    ctx,
    symbols: DbgHelpSymbols,
    verbose: bool,
) -> None:
    is_32 = isinstance(ctx, WOW64_CONTEXT)
    eip = ctx.Eip if is_32 else ctx.Rip
    esp = ctx.Esp if is_32 else ctx.Rsp
    ebp = ctx.Ebp if is_32 else ctx.Rbp

    code_hex = ex_rec.ExceptionCode_raw
    code_name = EXCEPTION_CODES.get(code_hex, "UNKNOWN_EXCEPTION")

    print_section("QUICK DIAGNOSIS")
    print_field("Exception", f"{code_name} (0x{code_hex:08x})", Colors.FAIL)
    crash_loc = format_module_offset(mf, ex_rec.ExceptionAddress)
    print_field("Crash at", crash_loc, Colors.FAIL + Colors.BOLD)

    mod, rva = resolve_address(mf, ex_rec.ExceptionAddress)
    if mod:
        mod_base = os.path.basename(mod.name).lower()
        if "ext_client" in mod_base:
            sym = symbols.lookup(ex_rec.ExceptionAddress)
            if sym:
                print_field("Symbol", sym, Colors.CYAN)
        hint = module_hint(mod.name, rva)
        if hint:
            print_field("Likely area", hint, Colors.WARNING)

    if code_hex == 0xC0000005:
        print_field("Details", explain_access_violation(ex_rec.ExceptionInformation), Colors.WARNING)

    # EBP chain (deduplicated, project modules first)
    print(f"\n{Colors.BOLD}EBP chain (call stack):{Colors.END}")
    ebp_frames = walk_ebp_chain(reader, mf, ebp)
    if ebp_frames:
        for i, addr in enumerate(ebp_frames):
            print(frame_line(mf, addr, symbols, prefix=f"  #{i:02d} "))
    else:
        print(f"  {Colors.DARK_GRAY}(EBP chain unavailable — using stack scan){Colors.END}")

    # Key frames: ext_client + sro_client only
    scan_limit = 2048 if verbose else 1024
    stack_frames = scan_stack_code_pointers(reader, mf, esp, scan_limit, not is_32)
    key_frames = [
        a for a in stack_frames
        if any(x in format_module_offset(mf, a).lower() for x in ("ext_client", "sro_client"))
    ]

    if key_frames:
        print(f"\n{Colors.BOLD}Key frames (ext_client / sro_client):{Colors.END}")
        shown: set[int] = set()
        for addr in key_frames:
            if addr in shown:
                continue
            shown.add(addr)
            print(frame_line(mf, addr, symbols))

    if not verbose:
        print(f"\n{Colors.DARK_GRAY}Tip: re-run with --verbose for registers, full stack scan, and module list.{Colors.END}")


def print_section(title: str) -> None:
    print(f"\n{Colors.HEADER}{Colors.BOLD}{'=' * 80}{Colors.END}")
    print(f"{Colors.HEADER}{Colors.BOLD} {title} {Colors.END}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'=' * 80}{Colors.END}")


def print_field(label: str, value: str, value_color: str = Colors.GREEN) -> None:
    print(f"{Colors.BOLD}{label:<22}:{Colors.END} {value_color}{value}{Colors.END}")


def print_hex_dump(reader, addr: int, size: int = 32, marker_addr: int | None = None) -> None:
    try:
        data = reader.read(addr, size)
    except Exception:
        print(f"  {Colors.DARK_GRAY}Memory at 0x{addr:x} not captured in dump.{Colors.END}")
        return
    line_len = 16
    for i in range(0, len(data), line_len):
        chunk = data[i : i + line_len]
        hex_str = " ".join(f"{b:02x}" for b in chunk)
        if len(chunk) < line_len:
            hex_str += " " * (line_len - len(chunk)) * 3
        ascii_str = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
        addr_str = f"0x{addr + i:08x}"
        print(f"  {Colors.BLUE}{addr_str}{Colors.END}  {hex_str}  |{Colors.CYAN}{ascii_str}{Colors.END}|")
        if marker_addr is not None and addr + i <= marker_addr < addr + i + len(chunk):
            offset = marker_addr - (addr + i)
            caret_pos = 12 + offset * 3
            print(" " * caret_pos + f"{Colors.FAIL}^{Colors.END}")


def main() -> None:
    args = [a for a in sys.argv[1:] if a not in ("--no-color", "--verbose")]
    verbose = "--verbose" in sys.argv

    if "--no-color" in sys.argv:
        Colors.disable()

    dmp_path = args[0] if args else find_latest_dmp()
    if not dmp_path:
        print(f"{Colors.FAIL}No .dmp found. Usage: python read_dmp.py [path.dmp]{Colors.END}")
        sys.exit(1)
    if not os.path.isfile(dmp_path):
        print(f"{Colors.FAIL}File not found: {dmp_path}{Colors.END}")
        sys.exit(1)

    print(f"\n{Colors.BOLD}Opening: {Colors.CYAN}{dmp_path}{Colors.END}")
    mf = MinidumpFile.parse(dmp_path)
    reader = mf.get_reader()
    symbols = DbgHelpSymbols(PROJECT_ROOT / "output" / "Release")

    if mf.exception is None:
        print(f"{Colors.FAIL}No exception stream in dump.{Colors.END}")
        sys.exit(1)

    ex_stream = mf.exception.exception_records[0]
    ex_rec = ex_stream.ExceptionRecord
    ctx = parse_context(mf, ex_stream.ThreadContext)

    mod, _ = resolve_address(mf, ex_rec.ExceptionAddress)
    if mod and "ext_client" in os.path.basename(mod.name).lower():
        symbols.rebase(mod.baseaddress)

    print_quick_diagnosis(mf, reader, ex_rec, ctx, symbols, verbose)

    if not verbose:
        print(f"\n{Colors.GREEN}{Colors.BOLD}Done.{Colors.END}\n")
        return

    # --- verbose sections below ---
    print_section("SYSTEM INFO")
    if mf.sysinfo:
        si = mf.sysinfo
        print_field("CPU", str(si.ProcessorArchitecture).split(".")[-1])
        print_field("OS", f"{si.MajorVersion}.{si.MinorVersion} build {si.BuildNumber}")

    is_32 = isinstance(ctx, WOW64_CONTEXT)
    eip = ctx.Eip if is_32 else ctx.Rip

    print_section("INSTRUCTION BYTES")
    print_hex_dump(reader, max(0, eip - 8), 32, marker_addr=eip)

    print_section("REGISTERS")
    if is_32:
        for name, val in [
            ("EIP", ctx.Eip), ("ESP", ctx.Esp), ("EBP", ctx.Ebp),
            ("EAX", ctx.Eax), ("EBX", ctx.Ebx), ("ECX", ctx.Ecx), ("EDX", ctx.Edx),
            ("ESI", ctx.Esi), ("EDI", ctx.Edi),
        ]:
            print(f"  {Colors.BOLD}{name:<4}{Colors.END}: {val:#010x}  ({format_module_offset(mf, val)})")

    print_section("FULL STACK SCAN")
    esp = ctx.Esp if is_32 else ctx.Rsp
    ptr_size = 4 if is_32 else 8
    try:
        stack_data = reader.read(esp, 2048)
        print(f"  {'Stack':<16} | {'Value':<16} | Location")
        print(f"  {'-'*16}-+-{'-'*16}-+-{'-'*30}")
        for offset in range(0, len(stack_data), ptr_size):
            ptr_val = int.from_bytes(stack_data[offset : offset + ptr_size], "little")
            mod, mod_off = resolve_address(mf, ptr_val)
            if not mod:
                continue
            base = os.path.basename(mod.name)
            print(f"  0x{esp + offset:08x} | 0x{ptr_val:08x} | {base}+0x{mod_off:x}")
    except Exception as exc:
        print(f"  {Colors.FAIL}{exc}{Colors.END}")

    print_section("MODULES (ext_client / sro_client)")
    if mf.modules:
        for mod in sorted(mf.modules.modules, key=lambda m: m.baseaddress):
            base = os.path.basename(mod.name).lower()
            if "ext_client" in base or "sro_client" in base:
                print(f"  0x{mod.baseaddress:08x}  {mod.size:#x}  {mod.name}")

    print(f"\n{Colors.GREEN}{Colors.BOLD}Done.{Colors.END}\n")


if __name__ == "__main__":
    main()
