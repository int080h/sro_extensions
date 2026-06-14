import sys
import os

def detect_encoding_and_read(filepath):
    if not os.path.exists(filepath):
        return None, None
    with open(filepath, 'rb') as f:
        content = f.read()
    if content.startswith(b'\xff\xfe'):
        return content.decode('utf-16'), 'utf-16'
    elif content.startswith(b'\xef\xbb\xbf'):
        return content.decode('utf-8-sig'), 'utf-8-sig'
    else:
        # Try UTF-8 first, fallback to cp1252 (ansi)
        try:
            return content.decode('utf-8'), 'utf-8'
        except UnicodeDecodeError:
            return content.decode('cp1252'), 'cp1252'

def write_with_encoding(filepath, content, encoding):
    with open(filepath, 'wb') as f:
        if encoding == 'utf-16':
            f.write(b'\xff\xfe') # UTF-16LE BOM
            f.write(content.encode('utf-16-le'))
        elif encoding == 'utf-8-sig':
            f.write(b'\xef\xbb\xbf') # UTF-8 BOM
            f.write(content.encode('utf-8'))
        else:
            f.write(content.encode(encoding))

def merge_questdata(existing_path, extracted_path):
    existing_content, encoding = detect_encoding_and_read(existing_path)
    if not existing_content:
        print(f"Error: Existing questdata file not found at: {existing_path}")
        return False
        
    extracted_content, _ = detect_encoding_and_read(extracted_path)
    if not extracted_content:
        print(f"Error: Extracted questdata file not found at: {extracted_path}")
        return False
        
    # Build set of existing Quest IDs
    existing_ids = set()
    for line in existing_content.splitlines():
        line = line.strip()
        if not line or line.startswith("//"):
            continue
        parts = line.split('\t')
        if len(parts) >= 2:
            quest_id = parts[1].strip()
            if quest_id.isdigit():
                existing_ids.add(int(quest_id))
                
    # Read extracted and check for new ones
    new_lines = []
    added_count = 0
    for line in extracted_content.splitlines():
        line_str = line.strip()
        if not line_str:
            continue
        parts = line_str.split('\t')
        if len(parts) >= 2:
            quest_id = parts[1].strip()
            if quest_id.isdigit() and int(quest_id) not in existing_ids:
                new_lines.append(line_str)
                existing_ids.add(int(quest_id))
                added_count += 1
                
    if added_count > 0:
        # Append new lines to existing content
        # Ensure there is a newline at the end of the existing content first
        separator = '\n'
        if not existing_content.endswith('\n'):
            existing_content += '\n'
        merged_content = existing_content + '\n'.join(new_lines) + '\n'
        write_with_encoding(existing_path, merged_content, encoding)
        print(f"[+] Merged {added_count} new quests into {existing_path}")
    else:
        print(f"[-] No new quests to merge into {existing_path} (already up-to-date).")
    return True

def merge_questcontentsdata(existing_path, extracted_path):
    existing_content, encoding = detect_encoding_and_read(existing_path)
    if not existing_content:
        print(f"Error: Existing questcontentsdata file not found at: {existing_path}")
        return False
        
    extracted_content, _ = detect_encoding_and_read(extracted_path)
    if not extracted_content:
        print(f"Error: Extracted questcontentsdata file not found at: {extracted_path}")
        return False
        
    # Build set of existing Quest CodeNames
    existing_codenames = set()
    for line in existing_content.splitlines():
        line = line.strip()
        if not line or line.startswith("//"):
            continue
        parts = line.split('\t')
        if len(parts) >= 1:
            codename = parts[0].strip()
            if codename:
                existing_codenames.add(codename)
                
    # Read extracted and check for new ones
    new_lines = []
    added_count = 0
    for line in extracted_content.splitlines():
        line_str = line.strip()
        if not line_str:
            continue
        parts = line_str.split('\t')
        if len(parts) >= 1:
            codename = parts[0].strip()
            if codename and codename not in existing_codenames:
                new_lines.append(line_str)
                existing_codenames.add(codename)
                added_count += 1
                
    if added_count > 0:
        if not existing_content.endswith('\n'):
            existing_content += '\n'
        merged_content = existing_content + '\n'.join(new_lines) + '\n'
        write_with_encoding(existing_path, merged_content, encoding)
        print(f"[+] Merged {added_count} new quest contents into {existing_path}")
    else:
        print(f"[-] No new quest contents to merge into {existing_path} (already up-to-date).")
    return True

def merge_queststrings(existing_path, extracted_path):
    existing_content, encoding = detect_encoding_and_read(existing_path)
    if not existing_content:
        print(f"Error: Existing queststrings file not found at: {existing_path}")
        return False
        
    extracted_content, _ = detect_encoding_and_read(extracted_path)
    if not extracted_content:
        print(f"Error: Extracted queststrings file not found at: {extracted_path}")
        return False
        
    # Build set of existing string keys
    existing_keys = set()
    for line in existing_content.splitlines():
        line = line.strip()
        if not line or line.startswith("//"):
            continue
        parts = line.split('\t')
        if len(parts) >= 1:
            existing_keys.add(parts[0].strip())
            
    # Read extracted and check for new ones
    new_lines = []
    added_count = 0
    for line in extracted_content.splitlines():
        line_str = line.strip()
        if not line_str:
            continue
        parts = line_str.split('\t')
        if len(parts) >= 1:
            key = parts[0].strip()
            if key not in existing_keys:
                new_lines.append(line_str)
                existing_keys.add(key)
                added_count += 1
                
    if added_count > 0:
        # Append new lines to existing content
        if not existing_content.endswith('\n'):
            existing_content += '\n'
        merged_content = existing_content + '\n'.join(new_lines) + '\n'
        write_with_encoding(existing_path, merged_content, encoding)
        print(f"[+] Merged {added_count} new translation keys into {existing_path}")
    else:
        print(f"[-] No new translation keys to merge into {existing_path} (already up-to-date).")
    return True

def main():
    if len(sys.argv) < 4:
        print("Usage: python merge_quest_files.py <existing_questdata_path> <existing_questcontentsdata_path> <existing_queststrings_path>")
        return
        
    existing_questdata = sys.argv[1]
    existing_questcontentsdata = sys.argv[2]
    existing_queststrings = sys.argv[3]
    
    extracted_questdata = "scratch/questdata_extracted.txt"
    extracted_questcontentsdata = "scratch/questcontentsdata_extracted.txt"
    extracted_queststrings = "scratch/queststrings_extracted.txt"
    
    if not os.path.exists(extracted_questdata) or not os.path.exists(extracted_questcontentsdata) or not os.path.exists(extracted_queststrings):
        print("Error: Extracted database files are missing. Please run extract_all_quests.py first.")
        return
        
    print("Starting merge process...")
    qdata_success = merge_questdata(existing_questdata, extracted_questdata)
    qcontents_success = merge_questcontentsdata(existing_questcontentsdata, extracted_questcontentsdata)
    qstrings_success = merge_queststrings(existing_queststrings, extracted_queststrings)
    
    if qdata_success and qcontents_success and qstrings_success:
        print("\n[+] Merge process completed successfully!")
    else:
        print("\n[-] Merge process encountered errors.")

if __name__ == "__main__":
    main()
