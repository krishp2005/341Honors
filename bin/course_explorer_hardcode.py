#!/usr/bin/env python3

directory = \
{
    "2025":
    {
        "Spring":
        {
            "CS":
            {
                "233": "Computer Architecture",
                "341": "System Programming",
                "425": "Distributed Systems",
            },
            "PHYS":
            {
                "211": "University Physics: Mechanics",
                "212": "University Physics: Elec & Mag",
            }
        }
    }
}

directories = \
[
    "2025",
    "2025/Spring",
    "2025/Spring/CS",
    "2025/Spring/PHYS",
]

files = \
[
    "2025/Spring/CS/233",
    "2025/Spring/CS/341",
    "2025/Spring/CS/425",
    "2025/Spring/PHYS/211",
    "2025/Spring/PHYS/212",
]

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

# the first flag determines what to do
# -s is search
# -d is checking if directory exists
# -f is checking if file exists
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

if __name__ == "__main__":
    main()
