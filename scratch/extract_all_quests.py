import subprocess
import os

def main():
    print("Connecting to database and extracting all active quests...")
    
    # We query the database in a single query by concatenating columns with tabs (CHAR(9))
    sql = """
    SET NOCOUNT ON;
    SELECT 
        CAST(Service AS VARCHAR) + CHAR(9) + 
        CAST(ID AS VARCHAR) + CHAR(9) + 
        CodeName + CHAR(9) + 
        CAST(Level AS VARCHAR) + CHAR(9) + 
        ISNULL(DescName, 'xxx') + CHAR(9) + 
        ISNULL(NameString, 'xxx') + CHAR(9) + 
        ISNULL(PayString, 'xxx') + CHAR(9) + 
        ISNULL(ContentsString, 'xxx') + CHAR(9) + 
        ISNULL(PayContents, 'xxx') + CHAR(9) + 
        ISNULL(NoticeNPC, 'xxx') + CHAR(9) + 
        ISNULL(NoticeCondition, 'xxx')
    FROM _RefQuest
    WHERE Service = 1
    ORDER BY ID ASC
    """
    
    cmd = [
        "sqlcmd",
        "-S", "localhost",
        "-E",
        "-C",
        "-d", "SILKROAD_R_SHARD",
        "-w", "8192",  # Allow wide lines to prevent truncation
        "-h", "-1",    # No headers
        "-Q", sql
    ]
    
    try:
        # Run sqlcmd capturing raw bytes
        res = subprocess.run(cmd, capture_output=True)
        if res.returncode != 0:
            print("SQLCMD Error:", res.stderr.decode('utf-8', errors='replace'))
            return
            
        # Try decoding with different encodings
        decoded_out = None
        for enc in ['cp1254', 'cp949', 'utf-8', 'latin1']:
            try:
                decoded_out = res.stdout.decode(enc)
                break
            except UnicodeDecodeError:
                continue
        if decoded_out is None:
            decoded_out = res.stdout.decode('utf-8', errors='replace')

        lines = decoded_out.strip().split('\n')
        valid_lines = []
        contents_lines = []
        string_keys = set()
        
        for line in lines:
            line_str = line.strip()
            if not line_str or line_str.startswith("(") or "affected" in line_str:
                continue
            
            # Since sqlcmd might add trailing padding spaces, we parse it into fields
            fields = line_str.split('\t')
            # Strip fields
            fields = [f.strip() for f in fields]
            
            # Reconstruct clean tab-separated line
            clean_line = '\t'.join(fields)
            valid_lines.append(clean_line)
            
            # Generate questcontentsdata line
            codename = fields[2]
            contents_line = f"{codename}\t0\t0\txxx\t1\tSN_CON_{codename}\txxx\txxx\txxx\txxx\txxx\txxx\txxx\t0\txxx\txxx\t0"
            contents_lines.append(contents_line)
            
            # Collect unique non-null string keys for translation
            # Columns: DescName(4), NameString(5), PayString(6), PayContents(8), NoticeNPC(9), NoticeCondition(10)
            for idx in [5, 6, 8, 9, 10]:
                if idx < len(fields):
                    key = fields[idx]
                    if key and key != "xxx" and key != "NULL":
                        string_keys.add((key, codename)) # tuple of (key, quest_codename)
            
            # Also add the content string key SN_CON_CodeName
            string_keys.add((f"SN_CON_{codename}", codename))

        # Write extracted quest data
        questdata_file = "scratch/questdata_extracted.txt"
        with open(questdata_file, "w", encoding="utf-16-le") as f:
            f.write("\ufeff")
            f.write('\n'.join(valid_lines) + '\n')
            
        # Write extracted quest contents data
        questcontentsdata_file = "scratch/questcontentsdata_extracted.txt"
        with open(questcontentsdata_file, "w", encoding="utf-16-le") as f:
            f.write("\ufeff")
            f.write('\n'.join(contents_lines) + '\n')
            
        # Write extracted string placeholders
        queststrings_file = "scratch/queststrings_extracted.txt"
        string_lines = []
        for key, codename in sorted(string_keys):
            string_lines.append(f'{key}\t"[Quest String: {codename}]"')
            
        with open(queststrings_file, "w", encoding="utf-16-le") as f:
            f.write("\ufeff")
            f.write('\n'.join(string_lines) + '\n')
            
        print(f"\n[+] Successfully extracted {len(valid_lines)} quests!")
        print(f"    - Quest definitions saved to: {questdata_file}")
        print(f"    - Quest contents definitions saved to: {questcontentsdata_file}")
        print(f"    - Quest translation keys saved to: {queststrings_file}")
        
    except Exception as e:
        print(f"Error executing extraction: {e}")

if __name__ == "__main__":
    main()
