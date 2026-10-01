#!/usr/bin/env python3
"""Generate packet_field_defs.hpp from silkroad-docs-main message markdown files.

Parses the "Structure Summary" code blocks from each message doc and produces
C++ static tables for runtime packet parsing.

Field types supported: u8, u16, u32, u64, i8, i16, i32, i64, f32, bool, ascii
Control structures: switch/case, for each (count-based loops), while ReadBool()
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from typing import Optional

ROOT = Path(__file__).resolve().parents[1]
MESSAGES = ROOT / "tools" / "silkroad-docs-main" / "docs" / "messages"
OUT = ROOT / "sro_ext_client" / "20-Program" / "plugins" / "net_log" / "packet_field_defs.hpp"

# SRO type -> (field_type enum, byte size)
TYPE_MAP = {
    "u8": ("u8", 1),
    "u16": ("u16", 2),
    "u32": ("u32", 4),
    "u64": ("u64", 8),
    "i8": ("i8", 1),
    "i16": ("i16", 2),
    "i32": ("i32", 4),
    "i64": ("i64", 8),
    "f32": ("f32", 4),
    "bool": ("bool_", 1),
    "ascii": ("ascii", 0),
    "string": ("ascii", 0),
    "raw": ("raw", 0),
}

# Sentinel for loop count reference: 0xFFFF means ReadBool-based loop
READ_BOOL_SENTINEL = 0xFFFF
# Sentinel for no branch value: -1
NO_BRANCH = -1
# Sentinel for no loop count ref: 0xFFFE
NO_LOOP = 0xFFFE


class FieldDef:
    __slots__ = ("name", "type_name", "size", "branch_value", "loop_count_ref", "children")

    def __init__(self, name: str, type_name: str, size: int = 0,
                 branch_value: int = NO_BRANCH, loop_count_ref: int = NO_LOOP):
        self.name = name
        self.type_name = type_name  # The enum string like "u8", "u16", "loop", "branch"
        self.size = size
        self.branch_value = branch_value
        self.loop_count_ref = loop_count_ref
        self.children: list[FieldDef] = []


class OpcodeDef:
    __slots__ = ("opcode", "name", "fields")

    def __init__(self, opcode: int, name: str):
        self.opcode = opcode
        self.name = name
        self.fields: list[FieldDef] = []


def extract_opcode(text: str) -> Optional[int]:
    m = re.search(r"Opcode\s*\|\s*`(0x[0-9A-Fa-f]+)`", text)
    if not m:
        return None
    return int(m.group(1), 16)


def extract_structure_summary(text: str) -> Optional[str]:
    """Extract the code block after '### Structure Summary'."""
    # Find the heading
    idx = text.find("### Structure Summary")
    if idx < 0:
        return None
    # Find the opening ``` after the heading
    code_start = text.find("```", idx)
    if code_start < 0:
        return None
    code_start += 3
    # Find the closing ```
    code_end = text.find("```", code_start)
    if code_end < 0:
        return None
    return text[code_start:code_end]


def extract_fields_table(text: str) -> Optional[list[tuple[str, str]]]:
    """Extract fields from the Fields table (fallback when no Structure Summary).
    Returns list of (name, type) tuples.
    Only uses the first (non-collapsed) Fields table.
    """
    # Find "### Fields" (possibly with suffix like " (Server RE)") that's NOT inside a <details> block
    details_idx = text.find("<details>")
    fields_idx = text.find("### Fields")
    if fields_idx < 0:
        return None
    # If there's a <details> before this Fields heading, skip to after </details>
    while details_idx >= 0 and details_idx < fields_idx:
        end_details = text.find("</details>", details_idx)
        if end_details < 0:
            return None
        fields_idx = text.find("### Fields", end_details)
        if fields_idx < 0:
            return None
        details_idx = text.find("<details>", end_details)

    # Parse the table - look for rows like | # | name | type | ...
    lines = text[fields_idx:].split("\n")
    fields = []
    for line in lines:
        line = line.strip()
        # Stop at <details> blocks (collapsed sections with different/inaccurate fields)
        if line.startswith("<details>"):
            break
        if line.startswith("|") and "---" not in line:
            cols = [c.strip() for c in line.split("|")]
            if len(cols) >= 4:
                name = cols[2].strip("`")
                type_str = cols[3].strip("`")
                if name and type_str and name != "Name" and not name.startswith("#"):
                    # Skip lines where the name itself contains if( or case(
                    if "if(" in name or "case(" in name:
                        continue
                    fields.append((name, type_str))
        elif line.startswith("###") and "Fields" not in line:
            break
    return fields if fields else None


def parse_type_from_text(type_str: str) -> tuple[str, int]:
    """Parse a type string like 'u32', 'u16/i32', 'ascii', 'f32x3' into (enum_name, size)."""
    type_str = type_str.strip().lower()
    # Handle union types like "u16/i32" - take the first one
    if "/" in type_str:
        type_str = type_str.split("/")[0].strip()
    # Remove qualifiers
    type_str = type_str.replace("var", "").strip()
    # Handle type multipliers like f32x3, u8x4, u32x2
    m = re.match(r'^(\w+?)(?:x(\d+))?$', type_str)
    if m:
        base = m.group(1)
        count = int(m.group(2)) if m.group(2) else 1
        if base in TYPE_MAP:
            enum_name, base_size = TYPE_MAP[base]
            return (enum_name, base_size * count)
    if type_str in TYPE_MAP:
        return TYPE_MAP[type_str]
    # Unknown type - default to raw
    return ("raw", 0)


def parse_structure_summary(summary: str) -> list[FieldDef]:
    """Parse the pseudo-structured-text from Structure Summary code blocks.

    Single-pass line-by-line parser. No nested while loops — every line is
    processed in one iteration with a simple state machine tracking indent
    level and current parent (loop/branch).
    """
    lines = summary.strip().split("\n")

    # State: stack of (parent_field, indent_level)
    # When we see a line with indent <= parent indent, we pop.
    root_fields: list[FieldDef] = []
    stack: list[tuple[FieldDef, int]] = []  # (parent, indent_of_parent_line)

    def current_parent() -> Optional[FieldDef]:
        return stack[-1][0] if stack else None

    def pop_to(indent: int) -> None:
        while stack and stack[-1][1] >= indent:
            stack.pop()

    for raw_line in lines:
        line = raw_line.rstrip()
        if not line.strip():
            continue
        stripped = line.strip()
        indent = len(line) - len(line.lstrip())

        # Skip comments
        if stripped.startswith("//"):
            continue

        # Skip IF/ELIF/ELSE conditionals — their children get parsed as regular fields
        if re.match(r'^(IF|ELIF|ELSE)\b', stripped, re.IGNORECASE):
            continue

        # Detect switch statement
        switch_m = re.match(r"switch\s+(\w+)\s*:", stripped)
        if switch_m:
            pop_to(indent)
            branch_field = FieldDef("switch", "branch", 0)
            if stack:
                stack[-1][0].children.append(branch_field)
            else:
                root_fields.append(branch_field)
            stack.append((branch_field, indent))
            continue

        # Detect case
        case_m = re.match(r"case\s+(\d+)\s*:\s*(.*)", stripped)
        if case_m:
            case_val = int(case_m.group(1))
            rest = case_m.group(2).strip()
            # Find the switch parent (should be on top of stack or close)
            pop_to(indent)
            if stack and stack[-1][0].type_name == "branch":
                if rest:
                    # Inline case: "case 1: Gold u64"
                    fields = parse_field_line(rest, case_val)
                    for field in fields:
                        stack[-1][0].children.append(field)
                else:
                    # Multi-line case — push a placeholder so following
                    # indented lines attach to this case
                    case_field = FieldDef(f"case_{case_val}", "branch", 0, branch_value=case_val)
                    stack[-1][0].children.append(case_field)
                    # Don't push to stack — following lines at deeper indent
                    # will be parsed as regular fields and attached to current parent
                    # Actually we need them under the switch, so just let them flow
            continue

        # Detect "default:" — skip
        if stripped.startswith("default"):
            continue

        # Detect "for each" loop
        for_each_m = re.match(r"for each\s*(\w+)?:?\s*(.*)", stripped, re.IGNORECASE)
        if for_each_m:
            pop_to(indent)
            loop_field = FieldDef("loop", "loop", 0)
            loop_field.loop_count_ref = READ_BOOL_SENTINEL
            # Try to find count field among siblings
            siblings = root_fields if not stack else stack[-1][0].children
            for prev in reversed(siblings):
                if "count" in prev.name.lower():
                    loop_field.name = f"loop_{prev.name}"
                    loop_field.loop_count_ref = len(siblings)  # hint, resolved later
                    break
            if stack:
                stack[-1][0].children.append(loop_field)
            else:
                root_fields.append(loop_field)
            stack.append((loop_field, indent))
            continue

        # Detect "while ReadBool()" loop
        while_m = re.match(r"while\s+ReadBool\s*\(\s*\)\s*:?", stripped, re.IGNORECASE)
        if while_m:
            pop_to(indent)
            loop_field = FieldDef("loop_readbool", "loop", 0)
            loop_field.loop_count_ref = READ_BOOL_SENTINEL
            if stack:
                stack[-1][0].children.append(loop_field)
            else:
                root_fields.append(loop_field)
            stack.append((loop_field, indent))
            continue

        # Regular field line(s) — parse_field_line returns a list
        fields = parse_field_line(stripped)
        if fields:
            pop_to(indent)
            for field in fields:
                if stack:
                    stack[-1][0].children.append(field)
                else:
                    root_fields.append(field)

    return root_fields


def _expand_slash_names(sub_names: list[str], enum_name: str, base_size: int,
                        branch_value: int = NO_BRANCH) -> list[FieldDef]:
    """Expand slash-separated name parts into individual FieldDefs.

    PosX/Y/Z → PosX, PosY, PosZ (replace last char of first name with each suffix)
    SpeedWalking/Running/Berserk → SpeedWalking, SpeedRunning, SpeedBerserk
        (prefix 'Speed' from first name + each suffix)
    """
    result = []
    first = sub_names[0].strip()
    result.append(FieldDef(first, enum_name, base_size, branch_value=branch_value))

    # Try to find a common prefix between first and subsequent names
    # For PosX/Y/Z: first='PosX', 'Y' → replace last char → 'PosY'
    # For SpeedWalking/Running/Berserk: first='SpeedWalking', 'Running' →
    #   common prefix 'Speed' + 'Running' → 'SpeedRunning'
    for si in range(1, len(sub_names)):
        sn = sub_names[si].strip()
        # Single char suffix: replace last char of first name
        if len(sn) == 1 and len(first) > 1:
            result.append(FieldDef(first[:-1] + sn, enum_name, base_size, branch_value=branch_value))
        else:
            # Find common prefix between first and sn
            common = 0
            while (common < len(first) and common < len(sn)
                   and first[common].lower() == sn[common].lower()):
                common += 1
            if common > 0:
                # Use common prefix + rest of sn
                result.append(FieldDef(first[:common] + sn[common:], enum_name, base_size, branch_value=branch_value))
            else:
                # No common prefix — just use the name as-is
                result.append(FieldDef(sn, enum_name, base_size, branch_value=branch_value))
    return result


def parse_field_line(text: str, branch_value: int = NO_BRANCH) -> list[FieldDef]:
    """Parse a field line and return a list of FieldDefs (can be multiple).

    Handles:
    - '[ 0] dwUniqueID u32' → single field
    - 'Name, JobName  ascii' → two fields with same type
    - 'RegionID, PosX/Y/Z, Angle  u16, f32x3, u16' → three fields
    - 'SpeedWalking/Running/Berserk  f32x3' → single field, size 12
    - 'GameState (BattleState)  u8' → single field, type u8
    - '[RidingUniqueID]  u32' → single field, conditional (brackets stripped)
    """
    text = text.strip()
    if not text or text.startswith("//") or text.startswith("switch") or text.startswith("for"):
        return []
    # Skip IF/ELIF/ELSE/ELSE (Regular): etc.
    if re.match(r'^(IF|ELIF|ELSE)\b', text, re.IGNORECASE):
        return []
    # Skip non-field lines starting with '(' like '(movement destination or action)' or '(variable)'
    if text.startswith("("):
        return []
    # Skip 'default:'
    if text.startswith("default"):
        return []

    # Remove inline comments
    if "//" in text:
        text = text[:text.index("//")].strip()

    # Remove leading [offset] bracket like '[ 0]', '[1]', or '[ ?]'
    text = re.sub(r"^\[\s*[\d?]+\s*\]\s*", "", text)

    # Split into tokens
    tokens = text.split()
    if len(tokens) < 2:
        return []

    # Known type prefixes
    type_prefixes = {'u8', 'u16', 'u32', 'u64', 'i8', 'i16', 'i32', 'i64',
                     'f32', 'bool', 'ascii', 'string', 'raw', 'var'}

    # Find the split point: first token that looks like a type
    split_idx = -1
    for i, tok in enumerate(tokens):
        tok_lower = tok.lower().rstrip(',')
        # Check if token is exactly a known type, or a type with multiplier (e.g. u32x3)
        for prefix in type_prefixes:
            if tok_lower == prefix or re.match(rf'^{re.escape(prefix)}x\d+$', tok_lower):
                split_idx = i
                break
        if split_idx >= 0:
            break

    if split_idx < 0:
        return []

    names_part = ' '.join(tokens[:split_idx])
    types_part = ' '.join(tokens[split_idx:])

    # Strip backtick formatting
    names_part = names_part.replace('`', '')
    types_part = types_part.replace('`', '')

    # Parse names: split by comma, strip whitespace and brackets
    names = []
    for n in names_part.split(','):
        n = n.strip()
        if not n:
            continue
        # Strip surrounding brackets like [RidingUniqueID]
        n = n.strip('[]')
        # Strip trailing parenthetical descriptions: 'GameState (BattleState)' → 'GameState'
        n = re.sub(r'\s*\([^)]*\)\s*', '', n).strip()
        if not n:
            continue
        # Skip if it's a description in parens like (BattleState)
        if n.startswith('('):
            continue
        # Skip pure description tokens
        if n.lower() in ('variable', 'data', 'info'):
            continue
        names.append(n)

    # Parse types: split by comma
    type_strs = [t.strip().strip('[]') for t in types_part.split(',') if t.strip().strip('[]')]

    if not names or not type_strs:
        return []

    # If single name with '/' separator and single type, split into multiple fields
    # e.g. 'SpeedWalking/Running/Berserk  f32' → 3 f32 fields
    # e.g. 'PosX/Y/Z  f32' → 3 f32 fields
    if len(names) == 1 and len(type_strs) == 1 and '/' in names[0]:
        sub_names = names[0].split('/')
        if len(sub_names) >= 2:
            t = type_strs[0]
            # Check for type multiplier like f32x3
            m = re.match(r'^(\w+?)x(\d+)$', t, re.IGNORECASE)
            if m:
                base = m.group(1).lower()
                count = int(m.group(2))
                if base in TYPE_MAP and count == len(sub_names):
                    enum_name, base_size = TYPE_MAP[base]
                    # Reconstruct names: PosX/Y/Z → PosX, PosY, PosZ
                    return _expand_slash_names(sub_names, enum_name, base_size, branch_value)
            # No multiplier — just use the base type for each split name
            enum_name, size = parse_type_from_text(t)
            return _expand_slash_names(sub_names, enum_name, size, branch_value)

    # If only one type but multiple names, apply same type to all names
    if len(type_strs) == 1 and len(names) > 1:
        t = type_strs[0]
        # Check if it's a type multiplier like u8x4 — expand: each name gets base type
        m = re.match(r'^(\w+?)x(\d+)$', t, re.IGNORECASE)
        if m:
            base = m.group(1).lower()
            count = int(m.group(2))
            if base in TYPE_MAP and count == len(names):
                enum_name, base_size = TYPE_MAP[base]
                return [FieldDef(n, enum_name, base_size, branch_value=branch_value) for n in names]
        enum_name, size = parse_type_from_text(t)
        return [FieldDef(n, enum_name, size, branch_value=branch_value) for n in names]

    # If same count of names and types, pair them
    if len(names) == len(type_strs):
        result = []
        for n, t in zip(names, type_strs):
            # Handle type multipliers like f32x3 → expand to multiple fields
            m = re.match(r'^(\w+?)x(\d+)$', t, re.IGNORECASE)
            if m:
                base = m.group(1).lower()
                count = int(m.group(2))
                if base in TYPE_MAP:
                    enum_name, base_size = TYPE_MAP[base]
                    if count <= 4 and '/' in n:
                        sub_names = n.split('/')
                        if len(sub_names) == count:
                            result.extend(_expand_slash_names(sub_names, enum_name, base_size, branch_value))
                        else:
                            result.append(FieldDef(n, enum_name, base_size * count, branch_value=branch_value))
                    else:
                        result.append(FieldDef(n, enum_name, base_size * count, branch_value=branch_value))
            else:
                # Check if name has slashes — expand even without multiplier
                if '/' in n:
                    sub_names = n.split('/')
                    enum_name, size = parse_type_from_text(t)
                    result.extend(_expand_slash_names(sub_names, enum_name, size, branch_value))
                else:
                    enum_name, size = parse_type_from_text(t)
                    result.append(FieldDef(n, enum_name, size, branch_value=branch_value))
        return result

    # Mismatched counts — pair greedily
    result = []
    for i, n in enumerate(names):
        if i < len(type_strs):
            enum_name, size = parse_type_from_text(type_strs[i])
        else:
            enum_name, size = parse_type_from_text(type_strs[-1])
        result.append(FieldDef(n, enum_name, size, branch_value=branch_value))
    return result


def flatten_fields(fields: list[FieldDef], indent: int = 0) -> list[tuple[FieldDef, int]]:
    """Flatten the field tree into a list with indent levels."""
    result = []
    for f in fields:
        result.append((f, indent))
        if f.children:
            result.extend(flatten_fields(f.children, indent + 1))
    return result


def resolve_loop_refs(fields: list[FieldDef]) -> None:
    """Try to resolve loop_count_ref by finding count fields by name."""
    for i, f in enumerate(fields):
        if f.type_name == "loop" and f.loop_count_ref != READ_BOOL_SENTINEL:
            # Look for a preceding field with "count" in the name
            for j in range(i - 1, -1, -1):
                if "count" in fields[j].name.lower():
                    f.loop_count_ref = j
                    break
            else:
                f.loop_count_ref = READ_BOOL_SENTINEL


def generate_cpp(opcodes: list[OpcodeDef]) -> str:
    """Generate the C++ header file content."""
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "// Generated by tools/gen_packet_defs.py from silkroad-docs-main — do not edit.",
        f"// {len(opcodes)} opcodes with field definitions.",
        "",
        "namespace ext_client::plugins::net_log::pkt {",
        "",
        "  enum class field_type : std::uint8_t {",
        "    u8, u16, u32, u64, i8, i16, i32, i64, f32, bool_, ascii, raw,",
        "    loop, branch,",
        "  };",
        "",
        "  struct field_def {",
        "    const char* name;",
        "    field_type type;",
        "    std::uint8_t size;",
        "    std::int32_t branch_value;",
        "    std::uint16_t loop_count_ref;",
        "    std::uint16_t child_start;",
        "    std::uint16_t child_count;",
        "  };",
        "",
        "  struct opcode_def {",
        "    std::uint16_t opcode;",
        "    const char* name;",
        "    std::uint16_t field_start;",
        "    std::uint16_t field_count;",
        "  };",
        "",
    ]

    # Flatten all fields into a single table
    all_fields: list[tuple[FieldDef, int, int]] = []  # (field, indent, parent_idx)
    flat_table: list[dict] = []  # Will hold the flat field defs

    def add_fields(fields: list[FieldDef], indent: int) -> int:
        """Add fields to the flat table, return the start index."""
        start = len(flat_table)
        for f in fields:
            idx = len(flat_table)
            child_start = 0
            child_count = 0
            if f.children:
                child_start = idx + 1
                # We need to add children right after, but we don't know count yet
                # So we add the parent first, then children
                flat_table.append({
                    "name": f.name,
                    "type": f.type_name,
                    "size": f.size,
                    "branch_value": f.branch_value,
                    "loop_count_ref": f.loop_count_ref,
                    "child_start": 0,  # will fix up
                    "child_count": len(f.children),
                })
                # Add children recursively
                child_idx = add_fields(f.children, indent + 1)
                # Fix up child_start
                flat_table[idx]["child_start"] = child_idx
            else:
                flat_table.append({
                    "name": f.name,
                    "type": f.type_name,
                    "size": f.size,
                    "branch_value": f.branch_value,
                    "loop_count_ref": f.loop_count_ref,
                    "child_start": 0,
                    "child_count": 0,
                })
        return start

    # Build opcode table
    opcode_entries = []
    for op in opcodes:
        field_start = len(flat_table)
        resolve_loop_refs(op.fields)
        add_fields(op.fields, 0)
        opcode_entries.append((op.opcode, op.name, field_start, len(flat_table) - field_start))

    # Emit opcode table
    lines.append("  inline constexpr opcode_def g_opcode_table[] = {")
    for opcode, name, fstart, fcount in opcode_entries:
        lines.append(f'    {{0x{opcode:04X}, "{name}", {fstart}, {fcount}}},')
    lines.append("  };")
    lines.append("")

    # Emit field table
    lines.append("  inline constexpr field_def g_field_table[] = {")
    for fd in flat_table:
        lines.append(
            f'    {{"{fd["name"]}", field_type::{fd["type"]}, {fd["size"]}, '
            f'{fd["branch_value"]}, {fd["loop_count_ref"]}, {fd["child_start"]}, {fd["child_count"]}}},'
        )
    lines.append("  };")
    lines.append("")

    # Emit lookup function
    lines.extend([
        "  inline auto lookup(std::uint16_t opcode) -> const opcode_def* {",
        "    for (const auto& def : g_opcode_table) {",
        "      if (def.opcode == opcode) {",
        "        return &def;",
        "      }",
        "    }",
        "    return nullptr;",
        "  }",
        "",
        "} // namespace ext_client::plugins::net_log::pkt",
        "",
    ])

    return "\n".join(lines)


def main() -> None:
    if not MESSAGES.exists():
        print(f"Error: messages directory not found: {MESSAGES}", file=sys.stderr)
        sys.exit(1)

    opcodes: list[OpcodeDef] = []
    skipped = 0

    for path in sorted(MESSAGES.glob("*.md")):
        if path.name == "INDEX.md":
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        opcode = extract_opcode(text)
        if opcode is None:
            skipped += 1
            continue

        name = path.stem
        summary = extract_structure_summary(text)
        summary_fields = parse_structure_summary(summary) if summary else []

        # Also try Fields table — use whichever produces more fields
        table_fields = extract_fields_table(text)
        table_parsed: list[FieldDef] = []
        if table_fields:
            for fname, ftype in table_fields:
                enum_name, size = parse_type_from_text(ftype)
                table_parsed.append(FieldDef(fname, enum_name, size))

        # Use whichever source has more fields
        if len(summary_fields) >= len(table_parsed) and summary_fields:
            fields = summary_fields
        elif table_parsed:
            fields = table_parsed
        else:
            fields = summary_fields  # could be empty

        if not fields:
            skipped += 1
            continue

        op_def = OpcodeDef(opcode, name)
        op_def.fields = fields
        opcodes.append(op_def)

    # Sort by opcode
    opcodes.sort(key=lambda o: o.opcode)

    # Generate output
    cpp = generate_cpp(opcodes)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(cpp, encoding="utf-8")
    print(f"wrote {len(opcodes)} opcodes ({skipped} skipped) to {OUT}")


if __name__ == "__main__":
    main()
