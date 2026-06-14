import random
import math
import subprocess
import csv
import io
import re

# Base region for raw coordinates
BASE_RX = 78
BASE_RZ = 105

def normalize(lx, ly, lz):
    return BASE_RX, BASE_RZ, lx, ly, lz

# Camera points: (X, Y, Z)
points = [
    (8319.42, 504.61, 468.39),
    (7380.32, 517.61, 190.49),
    (6254.55, 147.52, -34.03),
    (5498.98, 128.21, -99.35),
    (4754.90, 313.84, 729.90),
    (4497.97, 281.48, 1009.15),
    (3892.28, 133.26, 1054.60),
    (3245.52, 116.63, 1084.50),
    (3038.24, 100.67, 670.12),
    (2289.49, 124.66, 474.02),
    (1994.70, 274.24, 962.89),
    (1839.14, 488.47, 1603.73),
    (1522.07, 147.67, 2158.03),
    (1035.67, 126.86, 2134.77)
]

camera_angles = [
    (-0.979, 4.335, -0.007, 10),
    (0.351, 4.560, -0.007, 10),
    (0.091, 4.505, -0.418, 10),
    (-0.034, 5.120, -0.418, 10),
    (-0.154, 5.605, 0.071, 10),
    (0.196, 4.835, 0.528, 10),
    (0.016, 4.775, -0.228, 10),
    (0.001, 4.380, 0.015, 10),
    (-0.034, 5.220, -0.059, 10),
    (-0.079, 6.975, -0.059, 10),
    (0.181, 8.045, 0.018, 10),
    (0.326, 8.635, 0.018, 10),
    (-0.159, 7.675, 0.018, 10),
    (-0.334, 5.895, 0.018, 10)
]

camera_lines = []
for idx, (p, angles) in enumerate(zip(points, camera_angles)):
    t = idx * 3.0
    rx, rz = BASE_RX, BASE_RZ
    lx, ly, lz = p[0], p[1], p[2]
    yaw, pitch, roll, fov = angles
    camera_lines.append(f"0.0 S_CameraInsert {t:.1f} {rx} {rz} {lx:.2f} {ly:.2f} {lz:.2f} {yaw:.3f} {pitch:.3f} {roll:.3f} {fov}")

def fetch_db_characters(limit=300):
    sql = f"""
    SET NOCOUNT ON;
    WITH EquippedItems AS (
        SELECT 
            i.CharID,
            r.CodeName128,
            item.OptLevel,
            ROW_NUMBER() OVER(PARTITION BY i.CharID ORDER BY i.Slot) as rn
        FROM _Inventory i
        JOIN _Items item ON i.ItemID = item.ID64
        JOIN _RefObjCommon r ON item.RefItemID = r.ID
        WHERE i.Slot BETWEEN 0 AND 5
    )
    SELECT TOP {limit} c.CharName16, c.RefObjID, eq.CodeName128, ISNULL(eq.OptLevel, 0) AS OptLevel
    FROM _Char c
    LEFT JOIN EquippedItems eq ON c.CharID = eq.CharID AND eq.rn = 1
    WHERE c.Deleted = 0 AND c.CharID > 0
    ORDER BY NEWID()
    """
    
    cmd = [
        "sqlcmd",
        "-S", "localhost",
        "-E",
        "-C",
        "-d", "SILKROAD_R_SHARD",
        "-s", ",",
        "-W",
        "-Q", sql
    ]
    
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=True)
        lines = res.stdout.strip().split('\n')
        if not lines:
            return []
        
        cleaned_lines = []
        for line in lines:
            if re.match(r'^[-,\s]+$', line):
                continue
            cleaned_lines.append(line)
            
        reader = csv.DictReader(io.StringIO('\n'.join(cleaned_lines)))
        chars = []
        for row in reader:
            if not row.get('RefObjID'):
                continue
            chars.append({
                'CharName16': row['CharName16'],
                'RefObjID': int(row['RefObjID']),
                'CodeName128': row.get('CodeName128') or '',
                'OptLevel': int(row['OptLevel']) if row.get('OptLevel') else 0
            })
        return chars
    except Exception as e:
        print(f"Error executing sqlcmd: {e}")
        return []

def get_model_idx(ref_obj_id):
    if 1907 <= ref_obj_id <= 1919:
        return ref_obj_id - 1907
    elif 1920 <= ref_obj_id <= 1932:
        return 13 + (ref_obj_id - 1920)
    elif 14717 <= ref_obj_id <= 14729:
        return 26 + (ref_obj_id - 14717)
    elif 14730 <= ref_obj_id <= 14742:
        return 39 + (ref_obj_id - 14730)
    return random.randint(0, 51)  # Fallback

def get_equip_set(ref_obj_id, code_name, opt_level):
    if 1907 <= ref_obj_id <= 1932:
        race = 0
    else:
        race = 1
        
    armor_class = 3
    if not code_name:
        armor_class = 3
    elif '_HEAVY_' in code_name:
        armor_class = 1
    elif '_LIGHT_' in code_name:
        armor_class = 2
    elif '_CLOTHES_' in code_name or '_ROBE_' in code_name:
        armor_class = 3
        
    degree = 15
    if code_name:
        nums = re.findall(r'\d+', code_name)
        for n in nums:
            val = int(n)
            if 1 <= val <= 15:
                degree = val
                break
                
    plus = opt_level if opt_level is not None else 1
    plus = max(0, plus)
    
    return race, armor_class, degree, plus

print("Fetching characters from database...")
db_chars = fetch_db_characters(limit=300)
print(f"Fetched {len(db_chars)} characters.")

if not db_chars:
    print("Warning: No characters fetched from database. Falling back to default mock characters.")
    # Fallback mock characters
    for idx in range(150):
        db_chars.append({
            'CharName16': f'Fallback{idx}',
            'RefObjID': random.choice([1907, 1920, 14717, 14730]),
            'CodeName128': '',
            'OptLevel': 0
        })

script_lines = []
char_id = 1
db_char_idx = 0

# Generate characters along segments
for i in range(len(points) - 1):
    p1 = points[i]
    p2 = points[i+1]
    
    if i < 4:
        num_chars = 4  # sparse outside
    elif i < 9:
        num_chars = 8  # medium on approach roads
    else:
        num_chars = 20  # dense inside the city/square
        
    segment_start_time = i * 3.0
    
    for _ in range(num_chars):
        if db_char_idx >= len(db_chars):
            db_char_idx = 0  # Wrap around if needed
            
        char_data = db_chars[db_char_idx]
        db_char_idx += 1
        
        t = random.uniform(0.0, 1.0)
        x = p1[0] + t * (p2[0] - p1[0]) + random.uniform(-40.0, 40.0)
        y = p1[1] + t * (p2[1] - p1[1])
        z = p1[2] + t * (p2[2] - p1[2]) + random.uniform(-40.0, 40.0)
        
        # Ground height clamping
        char_y = 80.0
        if i >= 9:
            char_y = 83.73  # plaza square level
            
        model_idx = get_model_idx(char_data['RefObjID'])
        yaw = random.uniform(0, 360)
        
        # Spawn position
        rx, rz, lx, ly, lz = normalize(x, char_y, z)
        
        # Spawn at 0.0 seconds so they start loading immediately
        script_lines.append(f"0.0 S_ObjectCreateIndex char{char_id} {model_idx} {rx} {rz} {lx:.2f} {ly:.2f} {lz:.2f} {yaw:.2f} 1")
        
        # Equip set based on character equipment in DB
        race, armor_class, degree, plus = get_equip_set(
            char_data['RefObjID'], 
            char_data['CodeName128'], 
            char_data['OptLevel']
        )
        script_lines.append(f"0.0 S_ObjectEquipSetIndex char{char_id} {race} {armor_class} {degree} {plus}")
        
        # Random walk speed
        walk_speed = random.uniform(2.5, 4.5)
        script_lines.append(f"0.0 S_ObjectOptionSet char{char_id} *SCR_OBJ_WALKSPEED {walk_speed:.1f}")
        
        # Move destination (walk forward/backward/randomly)
        dest_x = x + random.uniform(-60.0, 60.0)
        dest_z = z + random.uniform(-60.0, 60.0)
        
        drx, drz, dlx, dly, dlz = normalize(dest_x, char_y, dest_z)
        
        trigger_time = max(2.0, segment_start_time - 2.0)
        script_lines.append(f"{trigger_time:.1f} S_ObjectMoveTo char{char_id} {drx} {drz} {dlx:.2f} {dly:.2f} {dlz:.2f}")
        
        char_id += 1

# Compile the final text
output = []
output.append("[CAMERA]")
output.append("AREA=EUROP_CONSTANTINOPLE")
output.extend(camera_lines)
output.append("[END]")
output.append("")
output.append("[SCRIPT]")
output.extend(script_lines)
output.append("[END]")

script_content = "\n".join(output) + "\n"

# Write to target file
paths = [
    r"C:\Users\alpka\Desktop\Game\sro_ext_client\tools\resinfo\constantinople.txt",
    r"C:\Users\alpka\Desktop\Game\sro_ext_client\script\intro\constantinople.txt"
]

import os
for path in paths:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        f.write(script_content)

print(f"Successfully generated {char_id-1} characters in script linked to database records.")
