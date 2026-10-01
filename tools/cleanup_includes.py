#!/usr/bin/env python3
"""Remove unnecessary offset includes from source files."""
import re
from pathlib import Path

root = Path(r"c:\Users\alpka\Desktop\Game\sro_extensions\sro_ext_client\src")
helpers = ['field_at', 'as_fn', 'global_at', 'raw_vftable', 'vtable_slot']

for f in sorted(root.rglob('*')):
    if f.suffix not in ('.cpp', '.hpp'):
        continue
    if 'offsets' in f.parts:
        continue
    content = f.read_text(encoding='utf-8', errors='replace')
    changed = False

    # Remove all.hpp includes
    if 'sdk/layout/offsets/all.hpp' in content:
        content = re.sub(r'#include\s+"sdk/layout/offsets/all\.hpp"\n?', '', content)
        changed = True

    # Check if file uses any helper functions
    uses_helpers = any(h in content for h in helpers)

    # Remove offsets.hpp include if no helpers are used
    if not uses_helpers and 'sdk/layout/offsets.hpp' in content:
        content = re.sub(r'#include\s+"sdk/layout/offsets\.hpp"\n?', '', content)
        changed = True

    if changed:
        f.write_text(content, encoding='utf-8')
        print(f'Updated: {f.relative_to(root)}')
