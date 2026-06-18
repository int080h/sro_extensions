"""Quick inspector for fire*.bms UV layout."""
import struct
import sys

def inspect(path):
    with open(path, "rb") as fs:
        data = fs.read()
    sig = data[:12].decode("ascii", errors="replace")
    print(f"sig: {sig!r}")

    # Offsets (10 uint32s after sig)
    vOff, sOff, fOff, c1, c2, bOff, oP, nM, sN, u9 = struct.unpack_from("<10I", data, 12)
    u0, nF, sP, vF, u2 = struct.unpack_from("<5I", data, 12 + 40)
    print(f"vOff={vOff} sOff={sOff} fOff={fOff} vF={vF:#x}")

    p = 12 + 40 + 20
    nLen = struct.unpack_from("<I", data, p)[0]
    p += 4 + nLen
    mLen = struct.unpack_from("<I", data, p)[0]
    p += 4
    mat = data[p:p+mLen].decode("ascii", errors="replace")
    p += mLen
    u3 = struct.unpack_from("<I", data, p)[0]
    p += 4

    print(f"material: {mat!r}")

    if vOff > 0:
        fs_pos = vOff
        vCount = struct.unpack_from("<I", data, fs_pos)[0]
        fs_pos += 4
        print(f"vertex count: {vCount}")
        for i in range(vCount):
            x,y,z, nx,ny,nz, u,v = struct.unpack_from("<8f", data, fs_pos)
            fs_pos += 32
            if vF & 0x400:
                fs_pos += 8
            if vF & 0x800:
                fs_pos += 36
            fs_pos += 4 + 4 + 4  # f0, i0, i1
            print(f"  v[{i}] pos=({x:.3f},{y:.3f},{z:.3f}) uv=({u:.3f},{v:.3f})")

    if fOff > 0:
        fs_pos = fOff
        fCount = struct.unpack_from("<I", data, fs_pos)[0]
        fs_pos += 4
        print(f"face count: {fCount}")
        for i in range(fCount):
            a,b,c = struct.unpack_from("<3H", data, fs_pos)
            fs_pos += 6
            print(f"  f[{i}] {a} {b} {c}")

if __name__ == "__main__":
    paths = sys.argv[1:] or [
        r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/meshes/fire.bms",
        r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/meshes/fire2.bms",
        r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/meshes/fire3.bms",
        r"C:/Users/alpka/Desktop/Game/sro_extensions/tools/Client_Rigid/Particles/meshes/fire4.bms",
    ]
    for p in paths:
        print("=" * 60)
        print(p)
        inspect(p)
