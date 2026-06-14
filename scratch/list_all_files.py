import glob
import os

files = glob.glob('ext_client/**/*.cpp', recursive=True) + glob.glob('ext_client/**/*.hpp', recursive=True)
files = sorted([f.replace("\\", "/") for f in files])

for f in files:
    print(f)
