#!/usr/bin/env python3
"""Parse all offset header files, build a complete value map, then inline replacements."""

import re
import os
import sys
from pathlib import Path

SRC_ROOT = Path(r"c:\Users\alpka\Desktop\Game\sro_extensions\sro_ext_client\src")
OFFSETS_DIR = SRC_ROOT / "sdk" / "layout" / "offsets"

def parse_offset_files():
    """Parse all offset .hpp files and build a map of fully_qualified_name -> (value_string, namespace_path)."""
    raw_map = {}  # e.g., "ext_client::off::cgwnd::visible" -> ("0x061", ["ext_client", "off", "cgwnd"])
    
    for hpp_file in sorted(OFFSETS_DIR.glob("*.hpp")):
        if hpp_file.name == "all.hpp":
            continue
        
        content = hpp_file.read_text(encoding="utf-8", errors="replace")
        lines = content.split("\n")
        ns_stack = []
        
        for line in lines:
            stripped = line.strip()
            
            # Track namespace opens - handle C++17 nested namespace syntax
            ns_match = re.match(r'namespace\s+(.+?)\s*\{', stripped)
            if ns_match:
                ns_full = ns_match.group(1).strip()
                parts = ns_full.split("::")
                ns_stack.extend(parts)
                continue
            
            # Track namespace closes
            if re.match(r'\}\s*//\s*namespace', stripped):
                ns_comment = re.match(r'\}\s*//\s*namespace\s+(.+)', stripped)
                if ns_comment:
                    ns_name = ns_comment.group(1).strip()
                    parts = ns_name.split("::")
                    for _ in parts:
                        if ns_stack:
                            ns_stack.pop()
                else:
                    if ns_stack:
                        ns_stack.pop()
                continue
            
            if stripped == '}':
                if ns_stack:
                    ns_stack.pop()
                continue
            
            # Track inline constexpr declarations
            constexpr_match = re.match(
                r'inline\s+constexpr\s+\S+(?:\s+\S+)*?\s+(\w+)\s*=\s*([^;]+);',
                stripped
            )
            if constexpr_match and ns_stack:
                name = constexpr_match.group(1)
                value = constexpr_match.group(2).strip()
                fq = '::'.join(ns_stack + [name])
                raw_map[fq] = (value, list(ns_stack))
    
    return raw_map

def resolve_value(value, ns_path, raw_map, depth=0, max_depth=20):
    """Resolve a value that might reference another constant (qualified or unqualified)."""
    if depth > max_depth:
        return value
    
    value = value.strip()
    
    # Raw hex
    if re.match(r'^0x[0-9A-Fa-f]+$', value):
        return value
    # Raw decimal
    if re.match(r'^\d+$', value):
        return value
    # String literal
    if value.startswith('"') or value.startswith("'"):
        return value
    
    # Try direct lookup (fully qualified)
    if value in raw_map:
        v, ns = raw_map[value]
        return resolve_value(v, ns, raw_map, depth + 1, max_depth)
    
    # Try unqualified lookup within the same namespace
    # e.g., value="unique_id", ns_path=["ext_client","off","cgwnd"]
    # Try "ext_client::off::cgwnd::unique_id"
    if ns_path:
        for i in range(len(ns_path), 0, -1):
            candidate = '::'.join(ns_path[:i] + [value])
            if candidate in raw_map:
                v, ns = raw_map[candidate]
                return resolve_value(v, ns, raw_map, depth + 1, max_depth)
    
    # Try looking up as a single name across all namespaces
    for fq, (v, ns) in raw_map.items():
        if fq.endswith("::" + value):
            return resolve_value(v, ns, raw_map, depth + 1, max_depth)
    
    return value

def build_resolved_map(raw_map):
    """Build a fully resolved map where all cross-references are resolved to raw values."""
    resolved = {}
    for key, (value, ns_path) in raw_map.items():
        resolved_val = resolve_value(value, ns_path, raw_map)
        resolved[key] = resolved_val
    return resolved

def generate_comment(name, value):
    """Generate a brief comment for an inlined offset."""
    # Extract the last part of the name for the comment
    parts = name.split("::")
    if len(parts) >= 2:
        short_name = parts[-1]
    else:
        short_name = name
    return f"/*{short_name}*/"

def replace_in_file(filepath, resolved_map):
    """Replace all ext_client::off:: references in a file with raw values."""
    content = filepath.read_text(encoding="utf-8", errors="replace")
    original = content
    
    # Sort keys by length (longest first) to avoid partial replacements
    sorted_keys = sorted(resolved_map.keys(), key=len, reverse=True)
    
    replacements_made = 0
    
    for key in sorted_keys:
        value = resolved_map[key]
        
        # Skip if value is the same as key (unresolved)
        if value == key:
            continue
        
        # Build comment from the short name
        short_name = key.split("::")[-1]
        comment = f"/*{short_name}*/"
        
        # Replace the key with value + comment
        # But only if the replacement value is different and is a raw value
        if re.match(r'^0x[0-9A-Fa-f]+$', value) or re.match(r'^\d+$', value) or value.startswith('"'):
            new_str = f"{value}{comment}"
        else:
            # Unresolved reference, skip
            continue
        
        # Count occurrences before replacing
        count = content.count(key)
        if count > 0:
            content = content.replace(key, new_str)
            replacements_made += count
    
    if content != original:
        filepath.write_text(content, encoding="utf-8")
        return replacements_made
    return 0

def main():
    print("=== Parsing offset files ===")
    raw_map = parse_offset_files()
    print(f"Found {len(raw_map)} raw constants")
    
    # Print some samples
    for k, v in sorted(raw_map.items())[:10]:
        print(f"  {k} = {v}")
    
    print("\n=== Resolving cross-references ===")
    resolved_map = build_resolved_map(raw_map)
    
    unresolved = {k: v for k, v in resolved_map.items() if v == k or not (re.match(r'^0x[0-9A-Fa-f]+$', v) or re.match(r'^\d+$', v) or v.startswith('"'))}
    if unresolved:
        print(f"WARNING: {len(unresolved)} unresolved references:")
        for k, v in sorted(unresolved.items()):
            print(f"  {k} = {v}")
    else:
        print("All references resolved!")
    
    print(f"\n=== Resolved map ({len(resolved_map)} entries) ===")
    
    # Print all resolved values for verification
    for k, v in sorted(resolved_map.items()):
        print(f"  {k} -> {v}")
    
    print("\n=== Replacing in source files ===")
    
    # Find all .cpp and .hpp files in src/ (excluding the offsets/ directory itself)
    all_files = []
    for ext in ["*.cpp", "*.hpp"]:
        for f in SRC_ROOT.rglob(ext):
            # Skip files in the offsets/ directory
            if "offsets" in f.parts:
                continue
            all_files.append(f)
    
    total_replacements = 0
    files_modified = 0
    
    for f in sorted(all_files):
        rel = f.relative_to(SRC_ROOT)
        count = replace_in_file(f, resolved_map)
        if count > 0:
            print(f"  {rel}: {count} replacements")
            total_replacements += count
            files_modified += 1
    
    print(f"\nTotal: {total_replacements} replacements across {files_modified} files")
    
    # Now update offsets.hpp to remove the include of all.hpp and the ext_client::offsets namespace
    offsets_hpp = SRC_ROOT / "sdk" / "layout" / "offsets.hpp"
    if offsets_hpp.exists():
        content = offsets_hpp.read_text(encoding="utf-8")
        # Remove the #include "sdk/layout/offsets/all.hpp" line
        content = content.replace('#include "sdk/layout/offsets/all.hpp"\n', '')
        # Remove the ext_client::offsets namespace block
        content = re.sub(
            r'namespace ext_client::offsets \{[^}]*\} // namespace ext_client::offsets',
            '',
            content,
            flags=re.DOTALL
        )
        offsets_hpp.write_text(content, encoding="utf-8")
        print(f"\nUpdated offsets.hpp")
    
    print("\n=== Done ===")
    print("Next steps:")
    print("1. Delete src/sdk/layout/offsets/ directory")
    print("2. Remove #include \"sdk/layout/offsets.hpp\" from files that no longer need it")
    print("3. Build and fix remaining errors")

if __name__ == "__main__":
    main()
