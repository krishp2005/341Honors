# 341Honors
Implementing a virtual filesystem (course_explorer_fs) allowing for local browsing of UIUC course information.

The file system is read only, supporting directory listings, as well as file readings.
An example directory would be `/2024/Fall/CS/341`, where inside stores the course description and instructors.

The primary benefit of having a FileSystem is that it is compatible with native programmar tools.
Furthermore, being a local cached copy (300s default), it is quick to index.
Possible uses cases include
1. Searching for course matches by keyword. 
    For example, running `grep -Hr Angrave **/CS` would list all courses that Professor Angrave has taught in the past 20 years.
2. Browing explorer using native file explorer, such as Finder (Mac).
3. Programmatically extending upon it, for example using the standard C API.

# Dependencies
1. libfuse [https://github.com/libfuse/libfuse]
2. xml parser (apt install python3-lxml)
3. pkg-config (apt install build-essential) OR (apt install pkg-config)

# Requirements
python3 -m pip install -r bin/requirements.txt

# Building and Running
The makefile provides all required commands, including 
- installation  (make install)
- mounting      (make mount)
- unmounting    (make unmount)
- clean         (make clean)
- uninstalling  (make uninstall)
