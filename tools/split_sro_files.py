import os
import sys
import argparse

def extract_id(line):
    """
    Attempts to extract an integer ID from an SRO text line.
    SRO data rows typically start with an index or ID (e.g., '1\t1000\t...')
    """
    cleaned = line.strip()
    if not cleaned or cleaned.startswith("//") or cleaned.startswith(";"):
        return None
        
    # SRO uses tabs for columns. We split by tab first to avoid splitting text sentences that contain spaces.
    parts = [p.strip() for p in cleaned.split('\t')]
    
    # If the first column is a loader flag (0 or 1), check the second column for ID
    if len(parts) > 1 and parts[0] in ('0', '1'):
        try:
            return int(parts[1])
        except ValueError:
            pass
            
    # Try first column
    try:
        return int(parts[0])
    except ValueError:
        pass
        
    # Try second column
    if len(parts) > 1:
        try:
            return int(parts[1])
        except ValueError:
            pass
            
    # Fallback to standard whitespace split if no tabs exist
    parts_space = cleaned.split()
    if len(parts_space) > 1 and parts_space[0] in ('0', '1'):
        try:
            return int(parts_space[1])
        except ValueError:
            pass
            
    for p in parts_space[:2]:
        try:
            return int(p)
        except ValueError:
            pass
            
    return None

def split_sro_file(merged_path, index_backup_path, output_dir=None, changed_dir=None):
    """
    Splits a merged SRO file back into its sub-files using the backup index file
    to determine sub-file names and original ID ranges.
    """
    if not os.path.exists(merged_path):
        print(f"Error: Merged file '{merged_path}' not found.")
        return False
        
    if not os.path.exists(index_backup_path):
        print(f"Error: Index backup file '{index_backup_path}' not found.")
        print("We need the backup index file to know what sub-files to create.")
        return False

    index_dir = os.path.dirname(os.path.abspath(merged_path))
    if not output_dir:
        output_dir = index_dir

    # 1. Parse the index backup file to get the list of split files in order
    sub_filenames = []
    encodings = ['utf-16', 'utf-8', 'cp1252']
    
    try:
        lines = []
        for encoding in encodings:
            try:
                with open(index_backup_path, 'r', encoding=encoding) as f:
                    lines = f.read().splitlines()
                break
            except UnicodeError:
                continue
                
        for line in lines:
            cleaned = line.strip()
            if not cleaned or cleaned.startswith("//") or cleaned.startswith(";"):
                continue
            if cleaned.lower().endswith('.txt'):
                sub_filenames.append(cleaned)
    except Exception as e:
        print(f"Error reading index backup: {e}")
        return False

    if not sub_filenames:
        print(f"Error: No sub-files listed in index backup '{index_backup_path}'.")
        return False

    print(f"Found {len(sub_filenames)} target sub-files from index backup.")

    # 2. Read the merged file lines
    merged_lines = []
    for encoding in encodings:
        try:
            with open(merged_path, 'r', encoding=encoding) as f:
                merged_lines = f.read().splitlines()
            break
        except UnicodeError:
            continue
            
    if not merged_lines:
        print(f"Error: Could not read merged file '{merged_path}' or file is empty.")
        return False

    # 3. Analyze original sub-files to map IDs to filenames (if original files still exist)
    id_to_file = {}
    file_to_header = {}
    has_originals = True
    
    print("Scanning original sub-files to build ID map...")
    for sf in sub_filenames:
        sf_path = os.path.join(index_dir, sf)
        if not os.path.exists(sf_path):
            has_originals = False
            break
            
        file_to_header[sf] = []
        sf_lines = []
        for encoding in encodings:
            try:
                with open(sf_path, 'r', encoding=encoding) as f:
                    sf_lines = f.read().splitlines()
                break
            except UnicodeError:
                continue
                
        for line in sf_lines:
            cleaned = line.strip()
            if not cleaned:
                continue
            # Store headers/comments separately to preserve them in each file
            if cleaned.startswith("//") or cleaned.startswith(";"):
                file_to_header[sf].append(line)
                continue
                
            row_id = extract_id(line)
            if row_id is not None:
                id_to_file[row_id] = sf

    # 4. Perform the split
    sub_file_contents = {sf: [] for sf in sub_filenames}
    
    if has_originals and id_to_file:
        print("Performing ID-based smart splitting...")
        # Add headers first
        for sf in sub_filenames:
            sub_file_contents[sf].extend(file_to_header.get(sf, []))
            
        unmapped_lines = []
        current_file = sub_filenames[0] # Default fallback
        
        for line in merged_lines:
            cleaned = line.strip()
            if not cleaned or cleaned.startswith("//") or cleaned.startswith(";"):
                continue # Skip comments in main data body to avoid duplicating them
                
            row_id = extract_id(line)
            if row_id is not None and row_id in id_to_file:
                target_file = id_to_file[row_id]
                sub_file_contents[target_file].append(line)
                current_file = target_file
            else:
                # If ID is new/unmapped, keep it in the same file as the previous row (sequential context)
                sub_file_contents[current_file].append(line)
    else:
        # Fallback: Split sequentially based on line distribution
        print("Original sub-files not found. Performing sequential line-count splitting...")
        total_lines = len(merged_lines)
        lines_per_file = total_lines // len(sub_filenames)
        
        for i, sf in enumerate(sub_filenames):
            start = i * lines_per_file
            end = (i + 1) * lines_per_file if i < len(sub_filenames) - 1 else total_lines
            sub_file_contents[sf] = merged_lines[start:end]

    # 5. Write the split files
    print("Writing split files...")
    modified_files = []
    unchanged_files = []
    
    # Ensure output_dir exists
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)
        
    for sf in sub_filenames:
        out_path = os.path.join(output_dir, sf)
        
        # Format the new content as a string
        new_content = "\n".join(sub_file_contents[sf]) + "\n"
        
        # Read existing file if it exists to check for changes
        is_changed = True
        if os.path.exists(out_path):
            existing_content = None
            for encoding in encodings:
                try:
                    with open(out_path, 'r', encoding=encoding) as f:
                        existing_content = f.read()
                    break
                except UnicodeError:
                    continue
            
            # If we successfully read existing content, compare it (normalize newlines)
            if existing_content is not None:
                if existing_content.replace('\r\n', '\n') == new_content.replace('\r\n', '\n'):
                    is_changed = False

        if is_changed:
            modified_files.append(sf)
            try:
                with open(out_path, 'w', encoding='utf-16') as f:
                    f.write(new_content)
                print(f"  Created/Updated: {sf} ({len(sub_file_contents[sf])} lines)")
            except Exception as e:
                print(f"  Error writing '{sf}': {e}")
        else:
            unchanged_files.append(sf)
            print(f"  Unchanged: {sf} (Skipped write)")

    # 6. If a separate changed-only directory is requested, write modified files there
    if changed_dir and modified_files:
        os.makedirs(changed_dir, exist_ok=True)
        print(f"\nCopying modified files only to: '{changed_dir}'")
        for sf in modified_files:
            dest_path = os.path.join(changed_dir, sf)
            src_path = os.path.join(output_dir, sf)
            try:
                # Read from output_dir and write to changed_dir
                with open(src_path, 'r', encoding='utf-16') as src, open(dest_path, 'w', encoding='utf-16') as dest:
                    dest.write(src.read())
                print(f"  Copied to changes: {sf}")
            except Exception as e:
                print(f"  Error copying '{sf}' to changes dir: {e}")

    print("\n" + "="*50)
    print("Splitting summary:")
    print(f"  Total sub-files: {len(sub_filenames)}")
    print(f"  Modified/Updated: {len(modified_files)}")
    print(f"  Unchanged: {len(unchanged_files)}")
    if modified_files:
        print("Modified files:")
        for sf in modified_files:
            print(f"    - {sf}")
    print("="*50)
    
    return True

def main():
    parser = argparse.ArgumentParser(description="Smart split a merged SRO file back into sub-files.")
    parser.add_argument("-m", "--merged", required=True, help="Path to the merged SRO file (e.g. skilldata.txt)")
    parser.add_argument("-b", "--backup", required=True, help="Path to the backup index file (e.g. skilldata_index_backup.txt)")
    parser.add_argument("-o", "--outdir", help="Output directory for split files (default: same as merged file)")
    parser.add_argument("-c", "--changed-dir", help="Output directory to copy ONLY the modified split files for easy patch package creation")

    args = parser.parse_args()
    
    split_sro_file(args.merged, args.backup, args.outdir, args.changed_dir)

if __name__ == "__main__":
    main()
