import subprocess
import csv
import io
import re

def fetch_db_characters():
    sql = """
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
    SELECT TOP 10 c.CharName16, c.RefObjID, eq.CodeName128, ISNULL(eq.OptLevel, 0) AS OptLevel
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
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            print("STDOUT:", res.stdout)
            print("STDERR:", res.stderr)
            return []
        # Parse output as CSV
        lines = res.stdout.strip().split('\n')
        if not lines:
            return []
        
        # Strip dashes line (usually second line: ----,---,---)
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

print(fetch_db_characters())
