"""Regenerate script/intro/constantinople.txt with camera-aligned NPC placement."""

from __future__ import annotations

import math
import os
import random
import re
import subprocess
import csv
import io

BASE_RX = 78
BASE_RZ = 105

CAMERA_POINTS = [
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
    (1035.67, 126.86, 2134.77),
]

CAMERA_ANGLES = [
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
    (-0.334, 5.895, 0.018, 10),
]

# Harbour / dock anchors (XZ from in-game dock; Y from live player sample ~-27).
HARBOR_ANCHORS = [
    (1490.0, -28.0, 1668.0, 90.0),
    (1420.0, -28.0, 1720.0, 45.0),
    (1560.0, -27.0, 1620.0, 200.0),
    (1380.0, -29.0, 1580.0, 120.0),
    (1650.0, -27.0, 1540.0, 270.0),
    (1720.0, -28.0, 1680.0, 315.0),
    (1280.0, -26.0, 2080.0, 60.0),
    (1180.0, -26.0, 2140.0, 140.0),
    (1080.0, -25.0, 2120.0, 220.0),
    (1340.0, -27.0, 1980.0, 0.0),
    (1480.0, -28.0, 1880.0, 180.0),
    (1600.0, -27.0, 1820.0, 250.0),
]

MODEL_POOL = [0, 1, 2, 4, 5, 6, 7, 9, 12, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24, 26, 27, 28, 29, 31, 32, 34, 35, 39, 40, 42, 43, 44, 46, 47, 48, 50]

EQUIP_POOL = [
    (0, 2, 8, 0),
    (0, 3, 1, 0),
    (1, 2, 8, 0),
    (1, 3, 1, 0),
    (1, 2, 10, 0),
    (0, 2, 1, 0),
    (1, 3, 5, 0),
]


def normalize(lx: float, ly: float, lz: float) -> tuple[int, int, float, float, float]:
    return BASE_RX, BASE_RZ, lx, ly, lz


def ground_y(x: float, z: float, camera_y: float, segment: int) -> float:
    # Calibrated from in-game samples: dock player Y~-27 when camera Y~488.
    jitter = random.uniform(-1.5, 1.5)
    if z >= 1350.0 and x <= 2200.0:
        return -28.0 + jitter
    if camera_y >= 400.0:
        return camera_y - 515.0 + jitter
    if camera_y >= 200.0:
        return camera_y - 270.0 + jitter
    if camera_y >= 100.0:
        return camera_y - 125.0 + jitter
    return camera_y - 95.0 + jitter


def fetch_db_characters(limit: int = 300) -> list[dict]:
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
    cmd = ["sqlcmd", "-S", "localhost", "-E", "-C", "-d", "SILKROAD_R_SHARD", "-s", ",", "-W", "-Q", sql]
    try:
        res = subprocess.run(cmd, capture_output=True, text=True, check=True)
        lines = res.stdout.strip().split("\n")
        cleaned = [line for line in lines if not re.match(r"^[-,\s]+$", line)]
        reader = csv.DictReader(io.StringIO("\n".join(cleaned)))
        chars = []
        for row in reader:
            if not row.get("RefObjID"):
                continue
            chars.append(
                {
                    "CharName16": row["CharName16"],
                    "RefObjID": int(row["RefObjID"]),
                    "CodeName128": row.get("CodeName128") or "",
                    "OptLevel": int(row["OptLevel"]) if row.get("OptLevel") else 0,
                }
            )
        return chars
    except Exception as exc:
        print(f"sqlcmd unavailable ({exc}); using built-in model pool")
        return []


def get_model_idx(ref_obj_id: int) -> int:
    if 1907 <= ref_obj_id <= 1919:
        return ref_obj_id - 1907
    if 1920 <= ref_obj_id <= 1932:
        return 13 + (ref_obj_id - 1920)
    if 14717 <= ref_obj_id <= 14729:
        return 26 + (ref_obj_id - 14717)
    if 14730 <= ref_obj_id <= 14742:
        return 39 + (ref_obj_id - 14730)
    return random.choice(MODEL_POOL)


def get_equip_set(ref_obj_id: int, code_name: str, opt_level: int) -> tuple[int, int, int, int]:
    race = 0 if 1907 <= ref_obj_id <= 1932 else 1
    armor_class = 3
    if "_HEAVY_" in code_name:
        armor_class = 1
    elif "_LIGHT_" in code_name:
        armor_class = 2
    elif "_CLOTHES_" in code_name or "_ROBE_" in code_name:
        armor_class = 3
    degree = 15
    if code_name:
        for n in re.findall(r"\d+", code_name):
            val = int(n)
            if 1 <= val <= 15:
                degree = val
                break
    plus = max(0, opt_level or 0)
    return race, armor_class, degree, plus


def append_char(
    lines: list[str],
    name: str,
    model_idx: int,
    x: float,
    y: float,
    z: float,
    yaw: float,
    equip: tuple[int, int, int, int],
    *,
    stationary: bool = False,
    walk_speed: float = 3.2,
    move_at: float | None = None,
    dest: tuple[float, float, float] | None = None,
) -> None:
    rx, rz, lx, ly, lz = normalize(x, y, z)
    lines.append(f"0.0 S_ObjectCreateIndex {name} {model_idx} {rx} {rz} {lx:.2f} {ly:.2f} {lz:.2f} {yaw:.2f} 1")
    lines.append(f"0.0 S_ObjectEquipSetIndex {name} {equip[0]} {equip[1]} {equip[2]} {equip[3]}")
    if stationary:
        lines.append(f"0.0 S_ObjectOptionSet {name} *SCR_OBJ_WALKSPEED 0.0")
        lines.append(f"0.0 S_ObjectOptionSet {name} SCR_OBJ_NOIDLE")
    else:
        lines.append(f"0.0 S_ObjectOptionSet {name} *SCR_OBJ_WALKSPEED {walk_speed:.1f}")
    if move_at is not None and dest is not None:
        drx, drz, dlx, dly, dlz = normalize(dest[0], dest[1], dest[2])
        lines.append(f"{move_at:.1f} S_ObjectMoveTo {name} {drx} {drz} {dlx:.2f} {dly:.2f} {dlz:.2f}")


def build_script(db_chars: list[dict]) -> str:
    random.seed(42)
    camera_lines = []
    for idx, (point, angles) in enumerate(zip(CAMERA_POINTS, CAMERA_ANGLES)):
        t = idx * 3.0
        yaw, pitch, roll, fov = angles
        camera_lines.append(
            f"0.0 S_CameraInsert {t:.1f} {BASE_RX} {BASE_RZ} {point[0]:.2f} {point[1]:.2f} {point[2]:.2f} {yaw:.3f} {pitch:.3f} {roll:.3f} {fov}"
        )

    script_lines: list[str] = []
    char_id = 1
    db_idx = 0

    def next_char_data() -> dict:
        nonlocal db_idx
        if not db_chars:
            return {"RefObjID": 0, "CodeName128": "", "OptLevel": 0}
        data = db_chars[db_idx % len(db_chars)]
        db_idx += 1
        return data

    # Fixed harbour crowd along the dock the camera sweeps at 30-39s.
    for idx, (x, y, z, yaw) in enumerate(HARBOR_ANCHORS):
        data = next_char_data()
        model = get_model_idx(data["RefObjID"])
        equip = get_equip_set(data["RefObjID"], data["CodeName128"], data["OptLevel"]) if db_chars else random.choice(EQUIP_POOL)
        stationary = idx % 3 == 0
        name = f"harbor{idx + 1}"
        dest = None if stationary else (x + random.uniform(-35, 35), y, z + random.uniform(-35, 35))
        append_char(
            script_lines,
            name,
            model,
            x + random.uniform(-8, 8),
            y,
            z + random.uniform(-8, 8),
            yaw + random.uniform(-20, 20),
            equip,
            stationary=stationary,
            walk_speed=random.uniform(2.4, 3.6),
            move_at=30.0 + idx * 0.4 if dest else None,
            dest=dest,
        )
        char_id += 1

    for segment, (p1, p2) in enumerate(zip(CAMERA_POINTS, CAMERA_POINTS[1:])):
        if segment < 4:
            count = 5
        elif segment < 8:
            count = 10
        elif segment < 10:
            count = 14
        else:
            count = 22

        segment_time = segment * 3.0
        for _ in range(count):
            data = next_char_data()
            t = random.uniform(0.05, 0.95)
            x = p1[0] + t * (p2[0] - p1[0]) + random.uniform(-55.0, 55.0)
            z = p1[2] + t * (p2[2] - p1[2]) + random.uniform(-55.0, 55.0)
            camera_y = p1[1] + t * (p2[1] - p1[1])
            y = ground_y(x, z, camera_y, segment)
            model = get_model_idx(data["RefObjID"])
            equip = get_equip_set(data["RefObjID"], data["CodeName128"], data["OptLevel"]) if db_chars else random.choice(EQUIP_POOL)
            yaw = random.uniform(0.0, 360.0)
            name = f"char{char_id}"
            stationary = segment >= 10 and random.random() < 0.35
            dest = None
            move_at = None
            if not stationary:
                dest = (x + random.uniform(-70, 70), y, z + random.uniform(-70, 70))
                move_at = max(2.0, segment_time)
            append_char(
                script_lines,
                name,
                model,
                x,
                y,
                z,
                yaw,
                equip,
                stationary=stationary,
                walk_speed=random.uniform(2.5, 4.2),
                move_at=move_at,
                dest=dest,
            )
            char_id += 1

    output = ["[CAMERA]", "AREA=EUROP_CONSTANTINOPLE", *camera_lines, "[END]", "", "[SCRIPT]", *script_lines, "[END]", ""]
    return "\n".join(output)


def main() -> None:
    db_chars = fetch_db_characters()
    content = build_script(db_chars)
    paths = [
        os.path.join(os.path.dirname(__file__), "..", "script", "intro", "constantinople.txt"),
        os.path.join(os.path.dirname(__file__), "..", "tools", "script", "intro", "constantinople.txt"),
    ]
    for path in paths:
        path = os.path.normpath(path)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(content)
        print(f"wrote {path}")


if __name__ == "__main__":
    main()
