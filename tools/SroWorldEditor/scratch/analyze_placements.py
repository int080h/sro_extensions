import struct
import os
from collections import Counter

client = r"c:\Users\alpka\Desktop\Game\sro_extensions\tools\Client_Rigid"
ifo = os.path.join(client, "Map", "object.ifo")
obj_map = {}
with open(ifo, "r", errors="ignore") as f:
    for line in f:
        line = line.strip()
        if not line or not line[0].isdigit():
            continue
        parts = line.split('"')
        if len(parts) < 2:
            continue
        oid = int(line.split()[0])
        path = parts[1]
        obj_map[oid] = path


def read_o2(path):
    with open(path, "rb") as f:
        f.read(12)
        objs = []
        for z in range(6):
            for x in range(6):
                for lod in range(4):
                    cnt = struct.unpack("<H", f.read(2))[0]
                    for _ in range(cnt):
                        oid = struct.unpack("<I", f.read(4))[0]
                        px, py, pz = struct.unpack("<fff", f.read(12))
                        is_static = struct.unpack("<h", f.read(2))[0]
                        yaw = struct.unpack("<f", f.read(4))[0]
                        uid = struct.unpack("<h", f.read(2))[0]
                        s0 = struct.unpack("<h", f.read(2))[0]
                        is_big = struct.unpack("<B", f.read(1))[0]
                        is_struct = struct.unpack("<B", f.read(1))[0]
                        rid = struct.unpack("<H", f.read(2))[0]
                        objs.append((oid, yaw, uid, is_big, is_struct, rid, px, py, pz, lod))
        return objs


for ry in [167, 168]:
    path = os.path.join(client, "Map", "97", f"{ry}.o2")
    objs = read_o2(path)
    ext = Counter()
    missing = 0
    cpd = 0
    bsr = 0
    rid0 = 0
    for o in objs:
        oid = o[0]
        p = obj_map.get(oid, "")
        if not p:
            missing += 1
        else:
            ext[os.path.splitext(p)[1].lower()] += 1
            if p.lower().endswith(".cpd"):
                cpd += 1
            elif p.lower().endswith(".bsr"):
                bsr += 1
        if o[5] == 0:
            rid0 += 1
    print(f"97,{ry}: objects={len(objs)} cpd={cpd} bsr={bsr} missing_id={missing} regionId0={rid0}")
    print("  ext", dict(ext))
    for s in objs:
        p = obj_map.get(s[0], "")
        if "tele" in p.lower() or "gate" in p.lower() or "portal" in p.lower():
            print("  teleport-ish", s[0], p, "yaw", round(s[1], 3), "rid", hex(s[5]))
