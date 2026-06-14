import subprocess
import sys

def fetch_column(quest_id, column):
    sql = f"SET NOCOUNT ON; SELECT ISNULL(CAST({column} AS NVARCHAR(MAX)), 'xxx') FROM _RefQuest WHERE ID = {quest_id}"
    cmd = ["sqlcmd", "-S", "localhost", "-E", "-C", "-d", "SILKROAD_R_SHARD", "-W", "-h", "-1", "-Q", sql]
    res = subprocess.run(cmd, capture_output=True, text=True, encoding='utf-8')
    if res.returncode != 0:
        return "xxx"
    val = res.stdout.strip()
    return val if val != "" else "xxx"

def main():
    if len(sys.argv) < 2:
        print("Usage: python sync_quests.py <quest_id>")
        return
        
    quest_id = sys.argv[1]
    
    # Check if quest exists first
    check_sql = f"SET NOCOUNT ON; SELECT COUNT(*) FROM _RefQuest WHERE ID = {quest_id}"
    cmd = ["sqlcmd", "-S", "localhost", "-E", "-C", "-d", "SILKROAD_R_SHARD", "-W", "-h", "-1", "-Q", check_sql]
    res = subprocess.run(cmd, capture_output=True, text=True, encoding='utf-8')
    if res.returncode != 0 or res.stdout.strip() == "0":
        print(f"Quest ID {quest_id} not found in database or SQL error.")
        return
        
    # Query each column to construct a perfect tab-separated line
    columns = [
        "Service", "ID", "CodeName", "Level", "DescName", 
        "NameString", "PayString", "ContentsString", "PayContents", 
        "NoticeNPC", "NoticeCondition"
    ]
    
    row_values = []
    for col in columns:
        val = fetch_column(quest_id, col)
        row_values.append(val)
        
    # Generate questdata.txt entry
    questdata_line = '\t'.join(row_values)
    
    # Write to a file to guarantee tabs are preserved (command prompts often expand tabs to spaces)
    out_filename = f"scratch/quest_export_{quest_id}.txt"
    with open(out_filename, "w", encoding="utf-8") as out_f:
        out_f.write(questdata_line + "\n\n")
        out_f.write(f"{row_values[5]}\t\"[Quest Name: {row_values[2]}]\"\n")
        out_f.write(f"{row_values[6]}\t\"[Quest Pay Info]\"\n")
        out_f.write(f"{row_values[8]}\t\"[Quest Pay Contents]\"\n")
        out_f.write(f"{row_values[9]}\t\"[Quest NPC Notice]\"\n")
        out_f.write(f"{row_values[10]}\t\"[Quest Notice Condition]\"\n")

    print("\n" + "="*80)
    print("1. ADD THIS TO questdata.txt:")
    print("="*80)
    print(questdata_line)
    print("="*80)
    
    # Generate queststrings.txt entry placeholders
    print("\n" + "="*80)
    print("2. ADD THIS TO queststrings.txt:")
    print("="*80)
    print(f"{row_values[5]}\t\"[Quest Name: {row_values[2]}]\"")
    print(f"{row_values[6]}\t\"[Quest Pay Info]\"")
    print(f"{row_values[8]}\t\"[Quest Pay Contents]\"")
    print(f"{row_values[9]}\t\"[Quest NPC Notice]\"")
    print(f"{row_values[10]}\t\"[Quest Notice Condition]\"")
    print("="*80)
    
    print(f"\n[!] Written to file: {out_filename} (Open this file to copy/paste to ensure tabs are preserved!)")
    
    print("\nInstructions:")
    print("1. Extract Media.pk2 using a PK2 extractor.")
    print("2. Open 'server_dep/silkroad/ddj/questdata.txt' and append the line from Step 1.")
    print("3. Open 'server_dep/silkroad/ddj/queststrings.txt' and append the lines from Step 2.")
    print("4. Repack the updated files into Media.pk2 using a PK2 writer.")

if __name__ == "__main__":
    main()
