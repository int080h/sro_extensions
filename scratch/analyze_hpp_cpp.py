import os

workspace = r"c:\Users\alpka\Desktop\Game\sro_ext_client\ext_client"

hpp_files = []
cpp_files = []

for root, dirs, files in os.walk(workspace):
    if 'third-party' in root or 'build' in root or '.git' in root:
        continue
    for file in files:
        filepath = os.path.join(root, file)
        rel_path = os.path.relpath(filepath, workspace)
        if file.endswith('.hpp') or file.endswith('.h'):
            hpp_files.append((rel_path, filepath))
        elif file.endswith('.cpp'):
            cpp_files.append((rel_path, filepath))

print("Total HPP/H files:", len(hpp_files))
print("Total CPP files:", len(cpp_files))

cpp_basenames = {os.path.splitext(os.path.basename(f))[0] for f, _ in cpp_files}

print("\nHPP/H files with NO corresponding CPP file:")
for rel_path, filepath in hpp_files:
    base = os.path.splitext(os.path.basename(rel_path))[0]
    if base not in cpp_basenames:
        print(f"  {rel_path}")

print("\nHPP/H files WITH corresponding CPP file:")
for rel_path, filepath in hpp_files:
    base = os.path.splitext(os.path.basename(rel_path))[0]
    if base in cpp_basenames:
        print(f"  {rel_path}")
