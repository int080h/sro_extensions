import os
import sys
import io
import argparse

# Force UTF-8 stdout/stderr on Windows to avoid UnicodeEncodeError in console output
if sys.platform.startswith('win') and hasattr(sys.stdout, 'buffer'):
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

def detect_encoding_and_read(file_path):
    """
    Attempts to read a file using UTF-16, UTF-8, or CP1252.
    """
    encodings = ['utf-16', 'utf-8', 'cp1252']
    for encoding in encodings:
        try:
            with open(file_path, 'r', encoding=encoding) as f:
                return f.read().splitlines(), encoding
        except UnicodeError:
            continue
    raise ValueError(f"Could not decode file '{file_path}' with UTF-16, UTF-8, or CP1252.")

def extract_key_and_columns(line):
    """
    Parses a tab-separated line. The key is extracted dynamically based on SRO format.
    Returns (key, columns_list) or (None, None).
    """
    cleaned = line.strip()
    # Skip empty lines, comments, or index listings (lines ending in .txt)
    if not cleaned or cleaned.startswith("//") or cleaned.startswith(";") or cleaned.lower().endswith(".txt"):
        return None, None
        
    parts = [p.strip() for p in cleaned.split('\t')]
    if not parts or not parts[0]:
        return None, None
        
    # Check if this matches SRO string file structure: flag (0/1) \t ID \t CODENAME \t ...
    if len(parts) >= 3 and parts[0] in ('0', '1') and parts[1].isdigit():
        return parts[1], parts
        
    # Standard SRO structures: ID \t CODENAME \t ...
    return parts[0], parts

def parse_sro_file(file_path):
    """
    Parses an SRO file into a dict mapping Key -> (raw_line, columns, line_num).
    """
    data = {}
    lines, encoding = detect_encoding_and_read(file_path)
    for idx, line in enumerate(lines, 1):
        key, cols = extract_key_and_columns(line)
        if key is not None:
            data[key] = (line, cols, idx)
    return data, encoding

def compare_files(file_a, file_b, verbose=False):
    """
    Compares two SRO text files.
    """
    try:
        data_a, enc_a = parse_sro_file(file_a)
        data_b, enc_b = parse_sro_file(file_b)
    except Exception as e:
        print(f"Error parsing files: {e}")
        return None

    keys_a = set(data_a.keys())
    keys_b = set(data_b.keys())

    added = keys_b - keys_a
    deleted = keys_a - keys_b
    common = keys_a & keys_b

    modified = []
    for k in common:
        raw_a, cols_a, line_num_a = data_a[k]
        raw_b, cols_b, line_num_b = data_b[k]
        if raw_a != raw_b:
            # Analyze column differences
            diffs = []
            max_len = max(len(cols_a), len(cols_b))
            for i in range(max_len):
                val_a = cols_a[i] if i < len(cols_a) else "<missing>"
                val_b = cols_b[i] if i < len(cols_b) else "<missing>"
                if val_a != val_b:
                    diffs.append((i, val_a, val_b))
            modified.append((k, diffs, raw_a, raw_b))

    return {
        "added": added,
        "deleted": deleted,
        "modified": modified,
        "total_a": len(keys_a),
        "total_b": len(keys_b),
        "enc_a": enc_a,
        "enc_b": enc_b,
        "data_a": data_a,
        "data_b": data_b
    }

def print_file_diff(file_a, file_b, diff, verbose=False, outfile=None):
    filename = os.path.basename(file_b)
    
    def log(text, console=True):
        if console:
            print(text)
        if outfile:
            outfile.write(text + "\n")

    log(f"\nComparing File: {filename}")
    log(f"  Source (Official): {file_a} ({diff['enc_a']})")
    log(f"  Target (Custom):   {file_b} ({diff['enc_b']})")
    log("-" * 50)
    log(f"  Rows in Source: {diff['total_a']}")
    log(f"  Rows in Target: {diff['total_b']}")
    log(f"  Added:          {len(diff['added'])}")
    log(f"  Deleted:        {len(diff['deleted'])}")
    log(f"  Modified:       {len(diff['modified'])}")
    log("-" * 50)

    # Define a safe sorting key that doesn't mix int and str types
    def safe_sort_key(k):
        return (0, int(k)) if k.isdigit() else (1, k)

    # Write details (if verbose is enabled, or always write to output file if output file is active!)
    write_details = verbose or (outfile is not None)
    
    # Only print verbose details to console if no output file is specified
    console_verbose = verbose and (outfile is None)

    if diff['added'] and write_details:
        log("\n  [+] Added Rows (Keys):", console=console_verbose)
        for k in sorted(diff['added'], key=safe_sort_key):
            raw_line, cols, line_num = diff['data_b'][k]
            log(f"    ID: {k} (Line {line_num} in Target): {raw_line}", console=console_verbose)

    if diff['deleted'] and write_details:
        log("\n  [-] Deleted Rows (Keys):", console=console_verbose)
        for k in sorted(diff['deleted'], key=safe_sort_key):
            raw_line, cols, line_num = diff['data_a'][k]
            log(f"    ID: {k} (Line {line_num} in Source): {raw_line}", console=console_verbose)

    if diff['modified'] and write_details:
        log("\n  [*] Modified Rows:", console=console_verbose)
        # If writing to file, write all. If printing to console without file, limit to 10
        display_limit = len(diff['modified']) if (outfile or verbose) else 10
        if not console_verbose and not outfile:
            display_limit = 10
            
        for k, col_diffs, raw_a, raw_b in sorted(diff['modified'], key=lambda x: safe_sort_key(x[0]))[:display_limit]:
            raw_a, cols_a, line_num_a = diff['data_a'][k]
            raw_b, cols_b, line_num_b = diff['data_b'][k]
            diff_desc = ", ".join([f"Col {col}: '{val_a}' -> '{val_b}'" for col, val_a, val_b in col_diffs])
            log(f"    ID {k} (Line {line_num_a} in Source -> Line {line_num_b} in Target): {diff_desc}", console=(console_verbose or (not outfile and display_limit == 10)))
            
        if len(diff['modified']) > display_limit and not outfile:
            log(f"    ... and {len(diff['modified']) - display_limit} more modified rows. Use --verbose or output file to see all.", console=True)

def compare_directories(dir_a, dir_b, verbose=False, outfile=None):
    """
    Compares two SRO directories, checking for missing/extra files and comparing contents of matching files.
    """
    def log(text, console=True):
        if console:
            print(text)
        if outfile:
            outfile.write(text + "\n")

    log(f"Comparing Directory A (Official): {dir_a}")
    log(f"Comparing Directory B (Custom):   {dir_b}")
    log("=" * 60)

    files_a = {f.lower(): f for f in os.listdir(dir_a) if f.lower().endswith(".txt")}
    files_b = {f.lower(): f for f in os.listdir(dir_b) if f.lower().endswith(".txt")}

    missing_in_target = sorted([files_a[f] for f in (files_a.keys() - files_b.keys())])
    extra_in_target = sorted([files_b[f] for f in (files_b.keys() - files_a.keys())])

    if missing_in_target:
        log("\n[-] Files Missing in Target (Exist in Official, but not in Custom):")
        for f in missing_in_target:
            log(f"  - {f}")
            
    if extra_in_target:
        log("\n[+] Extra Files in Target (Exist in Custom, but not in Official):")
        for f in extra_in_target:
            log(f"  - {f}")
            
    if missing_in_target or extra_in_target:
        log("=" * 60)

    common_files = sorted(list(files_a.keys() & files_b.keys()))

    if not common_files:
        log("No matching .txt files found in both directories to compare.")
        return

    summary = []
    for f in common_files:
        file_a = os.path.join(dir_a, files_a[f])
        file_b = os.path.join(dir_b, files_b[f])
        
        diff = compare_files(file_a, file_b, verbose)
        if diff:
            print_file_diff(file_a, file_b, diff, verbose, outfile)
            summary.append((files_b[f], len(diff['added']), len(diff['deleted']), len(diff['modified'])))
            log("=" * 60)

    log("\n" + "=" * 20 + " COMPARISON SUMMARY " + "=" * 20)
    log(f"{'Filename':<30} | {'Added':<8} | {'Deleted':<8} | {'Modified':<8}")
    log("-" * 60)
    for filename, add, delete, mod in summary:
        log(f"{filename:<30} | {add:<8} | {delete:<8} | {mod:<8}")
    log("=" * 60)

def main():
    parser = argparse.ArgumentParser(description="Compare SRO client text files/folders by matching entry IDs.")
    parser.add_argument("-s", "--source", required=True, help="Official/Original SRO folder or file path")
    parser.add_argument("-t", "--target", required=True, help="Your customized SRO folder or file path")
    parser.add_argument("-v", "--verbose", action="store_true", help="Print detailed additions and deletions")
    parser.add_argument("-o", "--output", help="Path to write the detailed comparison report file")

    args = parser.parse_args()

    outfile = None
    if args.output:
        try:
            outfile = open(args.output, 'w', encoding='utf-8')
            print(f"Detailed report will be written to: {args.output}")
        except Exception as e:
            print(f"Error opening output file '{args.output}': {e}")
            sys.exit(1)

    try:
        if os.path.isdir(args.source) and os.path.isdir(args.target):
            compare_directories(args.source, args.target, args.verbose, outfile)
        elif os.path.isfile(args.source) and os.path.isfile(args.target):
            diff = compare_files(args.source, args.target, args.verbose)
            if diff:
                print_file_diff(args.source, args.target, diff, args.verbose, outfile)
        else:
            print("Error: Source and Target must be both directories or both files.")
            sys.exit(1)
    finally:
        if outfile:
            outfile.close()
            print(f"Report successfully written to: {args.output}")

if __name__ == "__main__":
    main()
