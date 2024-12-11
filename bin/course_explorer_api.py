#!/usr/bin/env python3

import bs4
import requests
import sys
import struct
import time

url = "http://courses.illinois.edu/cisapp/explorer/schedule"
paths = \
[
    "calendarYear",
    "term",
    "subject",
    "course",
];
query = { path: None for path in paths }

def construct_url(*, calendarYear=None, term=None, subject=None, course=None, **kwargs):
    path = '/'.join(str(v) for v in (calendarYear, term, subject, course) if v is not None)
    if path == "":
        return f"{url}.xml"
    return f"{url}/{path}.xml"

def get(**kwargs):
    url = construct_url(**kwargs)
    r = requests.get(url)
    if r.status_code != 200:
        return None
    return bs4.BeautifulSoup(r.content.decode("utf-8"), "lxml-xml")

def get_soup(argc, argv):
    for i in range(1, argc):
        query[paths[i - 1]] = argv[i]
    return get(**query)

def search(argc, argv):
    soup = get_soup(argc, argv)
    if soup is None:
        return

    data = soup.find_all(paths[argc - 1])
    if argc == 2:
        return [entry.text.split(' ')[0] for entry in data]
    elif argc == 4:
        return [(entry["id"], entry.text) for entry in data]
    else:
        return [entry["id"] for entry in data]

def print_data(data: list):
    # 1) the number of files / directories inside
    # 2) the time (double) that it was last fetched -- consider using modify time of file
    # 3) an array of metadata items -- with pointer to where the variable length starts
    # 4) the variable length file content

    # struct metadata
    # {
    #     char filepath[4096];
    #     size_t contents_size;
    #     size_t contents_offset;
    #     int is_directory;
    #     int is_file;
    # };

    byte_data = [None] * len(data)
    is_file = isinstance(data[0], tuple)
    is_dir = isinstance(data[0], str)

    header = struct.pack('Nd', len(data), time.time())
    if is_file:
        byte_data = [None] * (2 * len(data))
        contents_offset = 16 + (4096 + 24) * len(data)
        for i, (d, t) in enumerate(data):
            m = struct.pack('4096sNNii', bytes(d, "ascii"), len(t), contents_offset, is_dir, is_file)
            byte_data[i] = m
            byte_data[i + len(data)] = struct.pack(f"{len(t)}sx", bytes(t, "ascii"))
            contents_offset += len(t) + 1
    else:
        for i, d in enumerate(data):
            m = struct.pack('4096sNNii', bytes(d, "ascii"), 0, 0, is_dir, is_file)
            byte_data[i] = m

    sys.stdout.buffer.write(header)
    for b in byte_data:
        sys.stdout.buffer.write(b)

def main():
    argc = len(sys.argv)
    argv = sys.argv

    if argc < 1 or argc > 4:
        return

    data = search(argc, argv)
    if data is None:
        return
    print_data(data)

if __name__ == "__main__":
    main()
