import os
import sys
import argparse

# List of common SRO files that are split (discovered via reverse engineering sro_client.exe)
COMMON_SRO_FILES = [
    "skilldata.txt",
    "characterdata.txt",
    "itemdata.txt",
    "refpackageitem.txt",
    "refscrapofpackageitem.txt",
    "refshopgoods.txt",
    "refpricepolicyofitem.txt",
    "magicoption.txt",
    "questdata.txt",
    "characterinfo.txt",
    "skillaniset.txt",
    "skilleffectset.txt",
    "charactervisualchange.txt",
    "textdata_object.txt",
    "textdata_equip&skill.txt",
    "textuisystem.txt",
    "textquest_speech&name.txt",
    "textquest_queststring.txt",
    "textzonename.txt",
    "textquest_otherstring.txt",
    "texthelp.txt",
    "textevent.txt"
]

def merge_index_file(index_path, output_path=None, overwrite=False, delete_split=False):
    """
    Reads an index file, finds the split files listed inside it,
    and merges their contents.
    """
    if not os.path.exists(index_path):
        print(f"Error: Index file '{index_path}' not found.")
        return False

    index_dir = os.path.dirname(os.path.abspath(index_path))
    index_filename = os.path.basename(index_path)

    # 1. Parse the index file to get the list of split files
    split_files = []
    try:
        # SRO text files are typically encoded in UTF-16 LE (with BOM) or ANSI/UTF-8.
        # We will attempt UTF-16 LE first, then fall back to UTF-8/ANSI.
        encodings = ['utf-16', 'utf-8', 'cp1252']
        lines = []
        for encoding in encodings:
            try:
                with open(index_path, 'r', encoding=encoding) as f:
                    lines = f.read().splitlines()
                break
            except UnicodeError:
                continue

        for line in lines:
            cleaned = line.strip()
            # Skip empty lines, comments, or headers
            if not cleaned or cleaned.startswith("//") or cleaned.startswith(";"):
                continue
            # If the line ends with .txt or matches a text filename pattern
            if cleaned.lower().endswith('.txt'):
                split_files.append(cleaned)
    except Exception as e:
        print(f"Error reading index file: {e}")
        return False

    if not split_files:
        print(f"Warning: No split files found listed in index file '{index_filename}'.")
        print("This file might already be merged or it's not a valid index file.")
        return False

    print(f"Found {len(split_files)} split files to merge from '{index_filename}':")
    for sf in split_files:
        print(f"  - {sf}")

    # 2. Determine output file path
    if not output_path:
        name, ext = os.path.splitext(index_path)
        if overwrite:
            # Overwrite the index file itself (rename index file first as backup)
            backup_path = name + "_index_backup" + ext
            try:
                os.rename(index_path, backup_path)
                print(f"Backed up index file to '{os.path.basename(backup_path)}'")
                output_path = index_path
            except Exception as e:
                print(f"Error backing up index file: {e}")
                return False
        else:
            output_path = name + "_merged" + ext

    # 3. Merge files
    print(f"Merging into '{output_path}'...")
    merged_count = 0
    try:
        # Open output file in UTF-16 LE (standard for SRO files) or UTF-8
        # We will use UTF-16 LE with BOM (utf-16) to ensure the client/server can read it
        with open(output_path, 'w', encoding='utf-16') as outfile:
            for i, sf in enumerate(split_files):
                sf_path = os.path.join(index_dir, sf)
                if not os.path.exists(sf_path):
                    print(f"Warning: Split file '{sf}' not found in '{index_dir}'. Skipping.")
                    continue

                # Read split file content
                content = None
                for encoding in encodings:
                    try:
                        with open(sf_path, 'r', encoding=encoding) as infile:
                            content = infile.read()
                        break
                    except UnicodeError:
                        continue

                if content is None:
                    print(f"Error: Could not read split file '{sf}' with any standard encoding. Skipping.")
                    continue

                # Write content to merged file
                outfile.write(content)
                # Ensure there is a newline between files
                if not content.endswith('\n'):
                    outfile.write('\n')
                merged_count += 1
                print(f"  [{merged_count}/{len(split_files)}] Merged {sf}")

        print(f"Successfully merged {merged_count} files into '{output_path}'.")
        
        # Delete split files if requested (only if merging in-place / overwriting original index)
        if delete_split and merged_count > 0:
            if overwrite or (output_path and os.path.abspath(output_path) == os.path.abspath(index_path)):
                print("Deleting individual split files...")
                for sf in split_files:
                    sf_path = os.path.join(index_dir, sf)
                    if os.path.exists(sf_path):
                        try:
                            os.remove(sf_path)
                            print(f"  Deleted: {sf}")
                        except Exception as e:
                            print(f"  Warning: Could not delete '{sf}': {e}")
            else:
                print("Skipping deletion of split files because output path is different from index path (safety check).")
                
        return True
    except Exception as e:
        print(f"Error during merge: {e}")
        return False

def auto_scan_dir(directory, out_dir=None, overwrite=False, delete_split=False):
    """
    Scans a directory for common SRO index files and merges them.
    """
    print(f"Scanning directory '{directory}' for SRO index files...")
    found_any = False
    for filename in os.listdir(directory):
        if filename.lower() in COMMON_SRO_FILES:
            index_path = os.path.join(directory, filename)
            output_path = None
            if out_dir:
                os.makedirs(out_dir, exist_ok=True)
                output_path = os.path.join(out_dir, filename)
            print("-" * 50)
            print(f"Processing index file: {filename}")
            merge_index_file(index_path, output_path=output_path, overwrite=overwrite, delete_split=delete_split)
            found_any = True
    
    if not found_any:
        print("No common SRO index files found in the directory.")

def main():
    parser = argparse.ArgumentParser(description="Merge SRO split text files based on their index file.")
    parser.add_argument("-i", "--index", help="Path to the SRO index file (e.g. skilldata.txt)")
    parser.add_argument("-o", "--output", help="Custom output file path (default: [index]_merged.txt)")
    parser.add_argument("-d", "--dir", help="Directory path to scan and merge all common SRO index files")
    parser.add_argument("--overwrite", action="store_true", help="Overwrite the original index file (replaces it with merged content, creating a backup)")
    parser.add_argument("--delete", action="store_true", help="Delete the individual split files after successful merge")
    parser.add_argument("--out-dir", help="Custom output directory to save merged files (instead of same directory)")

    args = parser.parse_args()

    if args.dir:
        auto_scan_dir(args.dir, out_dir=args.out_dir, overwrite=args.overwrite, delete_split=args.delete)
    elif args.index:
        merge_index_file(args.index, output_path=args.output, overwrite=args.overwrite, delete_split=args.delete)
    else:
        # Default behavior: scan the current working directory
        auto_scan_dir(".", out_dir=args.out_dir, overwrite=args.overwrite, delete_split=args.delete)

if __name__ == "__main__":
    main()
