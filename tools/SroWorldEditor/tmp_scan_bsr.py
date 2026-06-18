"""Look for bsr/efp pairs that reference fire001.ddj or fire.bms."""
import struct, glob, os, sys

def scan_strings(data):
    strings = []
    for i in range(len(data) - 4):
        n = struct.unpack_from("<I", data, i)[0]
        if 4 <= n <= 260 and i + 4 + n <= len(data):
            try:
                s = data[i+4:i+4+n].decode("ascii")
            except UnicodeDecodeError:
                continue
            if all(32 <= ord(c) < 127 for c in s):
                strings.append(s)
    return strings

paths = (
    glob.glob(r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Data/res/bldg/**/*fire*.bsr", recursive=True)
    + glob.glob(r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Data/res/bldg/**/*brazier*.bsr", recursive=True)
    + glob.glob(r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Data/res/item/etc/campfire.bsr")
    + glob.glob(r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/dun/fire_animation_etc_slow*.efp")
    + glob.glob(r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/dun/demon_flame_fire*.efp")
)

for p in paths:
    with open(p, "rb") as f:
        data = f.read()
    strings = scan_strings(data)
    interesting = [s for s in strings if s.endswith((".efp", ".bms", ".ddj", ".dds")) and ("fire" in s.lower() or "brazier" in s.lower() or "flame" in s.lower() or "torch" in s.lower() or "camp" in s.lower() or "bonfire" in s.lower() or "torch" in s.lower())]
    if interesting:
        print(p)
        for s in sorted(set(interesting)):
            print("  ", s)
