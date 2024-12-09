#!/usr/bin/env python3

import bs4
import requests
import sys

url = "http://courses.illinois.edu/cisapp/explorer/schedule"
paths = \
[
    "calendarYear",
    "term",
    "subject",
    "course",
    "label"
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
    for i in range(2, argc):
        query[paths[i - 2]] = argv[i]
    return get(**query)

def search(argc, argv):
    soup = get_soup(argc, argv)
    if soup is None:
        return

    data = soup.find_all(paths[argc - 2])
    if argc == 3:
        data = [entry.text.split(' ')[0] for entry in data]
    elif argc == 6:
        data = [entry.text for entry in data]
    else:
        data = [entry["id"] for entry in data]
    return '\n'.join(data)

def main():
    argc = len(sys.argv)
    argv = sys.argv

    if argc < 2 or argc > 6:
        return

    search(argc, argv)

if __name__ == "__main__":
    main()
