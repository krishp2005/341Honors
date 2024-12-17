# 341Honors
Implementing a virtual filesystem through FUSE allowing for local browsing of UIUC course information.
Queries a Course Explorer API endpoint [http://courses.illinois.edu/cisapp/explorer/schedule.html] to receive data

The file system is read only, supporting directory listings, as well as file readings.
An example directory would be `/2024/Fall/CS/341`, where inside stores the course description and instructors.

The primary benefit of having a FileSystem is that it is compatible with native programmar tools.
Furthermore, being a local cached copy (300s default), it is quick to index.
Possible uses cases include
1. Searching for course matches by keyword. 
    For example, running `grep -Hr Angrave **/CS` would list all courses that Professor Angrave has taught in the past 20 years.
2. Browing explorer using native file explorer, such as Finder (Mac).
3. Programmatically extending upon it, for example using the standard C API.

# Roles
Ian Chen (ianchen3):
- Implemented `stat`, `readdir`, `read`, `open` system calls 
- Implemented python script to query API endpoint
- Designed and implemented metadata scheme allowing persistent caching
- Wrote installation/build scripts, project documentation

Krish Patwari (krishp3) :
Proposed the idea for the project by drawing inspiration from the Filesystem MP.Suggested integrating real-time course data using the UIUC Course Explorer API to make the project more dynamic and practical.Optimized data fetching using mmap for metadata files and ensured proper memory cleanup.Developed robust networking calls using Python's requests library to interact with the Course Explorer API.Implemented error handling for network timeouts, invalid responses, and ensured retries for failed requests.Improved the efficiency of system call implementations (getattr, readdir, etc.) by reducing unnecessary I/O operations.Assisted Ian with debugging system call functions and fixing edge cases in fill_directory_contents() and map_metadata().Wrote test cases to verify functionality, ensuring correct handling of invalid paths, stale cache data, and broken API responses.Played a key role in preparing the presentation, showcasing the project architecture, challenges,and optimizations.

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
