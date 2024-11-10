#!/usr/bin/env python3

import sys

def search(argc, argv):
    current = directory
    for i in range(2, argc):
        cur = argv[i].strip()
        if isinstance(current, str):
            current = current[cur]
        if isinstance(current, dict):
            current = current[cur]

    if isinstance(current, str):
        print(f"F{current}")
    if isinstance(current, dict):
        dirents = '\n'.join(current.keys())
        print(f"D{dirents}")

def is_directory(argc, argv):
    direc = argv[2]
    if direc in directories:
        print(1)
    else:
        print(0)

def is_file(argc, argv):
    f = argv[2]
    if f in files:
        print(1)
    else:
        print(0)

def list_direc(argc, argv):
    print('\n'.join(directories))

def list_files(argc, argv):
    print('\n'.join(files))

# the first flag determines what to do
# -s is search
# -d is checking if directory exists
# -f is checking if file exists
# -lf is listing all files
# -ld is listing all directories
def main():
    argc = len(sys.argv)
    argv = sys.argv

    if argc < 2:
        return

    if argv[1] == "-s":
        search(argc, argv)
    if argv[1] == "-d":
        is_directory(argc, argv)
    if argv[1] == "-f":
        is_file(argc, argv)
    if argv[1] == "-lf":
        list_files(argc, argv)
    if argv[1] == "-ld":
        list_direc(argc, argv)

if __name__ == "__main__":
    main()
