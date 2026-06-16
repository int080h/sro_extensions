import struct
import os

def read_string(f):
    ln = struct.unpack('<I', f.read(4))[0]
    if ln == 0:
        return ''
    return f.read(ln).decode('latin1', errors='replace')

def parse_cpd(path):
    with open(path, 'rb') as f:
        sig = f.read(12).decode('ascii', errors='replace')
        print('sig', sig)
        hdr = struct.unpack('<7I', f.read(28))
        print('hdr', hdr)
        typ = struct.unpack('<I', f.read(4))[0]
        name = read_string(f)
        u5, u6 = struct.unpack('<2I', f.read(8))
        print('type', typ, 'name', name, 'u5', u5, 'u6', u6)
        pos = f.tell()
        print('at', pos)
        # try collision at hdr[0]
        f.seek(hdr[0])
        col = read_string(f)
        print('collision@off', col)
        f.seek(hdr[1])
        cnt = struct.unpack('<I', f.read(4))[0]
        print('resource count', cnt)
        for i in range(cnt):
            p = read_string(f)
            print(' ', i, p)

client = r'c:\Users\alpka\Desktop\Game\sro_extensions\tools\Client_Rigid'
# one cpd from region 168
ifo = os.path.join(client, 'Map', 'object.ifo')
cpds = []
with open(ifo) as f:
    for line in f:
        if '.cpd' in line.lower():
            parts = line.split('"')
            if len(parts) >= 2:
                cpds.append(parts[1])
for rel in cpds[:3]:
    path = os.path.join(client, rel.replace('\\', os.sep))
    if not os.path.exists(path):
        path = os.path.join(client, 'Data', rel.replace('\\', os.sep))
    print('===', rel, 'exists', os.path.exists(path))
    if os.path.exists(path):
        parse_cpd(path)
