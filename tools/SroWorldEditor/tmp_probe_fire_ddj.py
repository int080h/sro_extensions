"""Probe other fire textures to confirm the 4x3 / 3x4 grid layout assumption."""
import struct, sys

def read_ddj_size(path):
    """Read the JMXVDDJ header to extract width/height."""
    with open(path, "rb") as f:
        data = f.read(64)
    if data[:7] != b"JMXVDDJ":
        return None
    # Skip the format tag
    if data[7:11] == b"DXT1":
        bpp = 4
    elif data[7:11] == b"DXT5":
        bpp = 8
    elif data[7:11] == b"\x00\x00\x00\x00":
        bpp = 4
    else:
        bpp = 4
    return {"fmt": data[7:11], "data": data}

textures = [
    r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/textures/fire001.ddj",
    r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/textures/fire.ddj",
    r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/textures/fire-2.ddj",
    r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/textures/fire2.ddj",
    r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/textures/fire16.ddj",
    r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/textures/fireball.ddj",
    r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/textures/fire_spot_01.ddj",
    r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/textures/fire_spot_03.ddj",
    r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/textures/fire_pong.ddj",
]

for p in textures:
    info = read_ddj_size(p)
    print(p.rsplit("/",1)[1], info)
