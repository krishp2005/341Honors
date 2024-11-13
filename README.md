# 341Honors
Course explorer filesystem on RPI5.
Implementing a virtual filesystem (course_explorer_fs) allowing for local
browsing of UIUC course information.

# Dependencies
1. libfuse [https://github.com/libfuse/libfuse]
2. xml parser (apt install python3-lxml)
3. pkg-config (apt install build-essential) OR (apt install pkg-config)

# Pre-Build Instructions
make install

# Build Instructions
cd naive_fs && make_mount

# Uninstallation
cd naive_fs && make unmount
make uninstall
