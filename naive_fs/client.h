#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "vector.h"

// #define DIR_EXE ("course_explorer_hardcode.py")
#define DIR_EXE ("cs341_course_explorer_api.py")

struct dir_listing
{
    /**
     * vector of char *
     * each directory entry is going to be a canonical path
     */
    vector *dirents;
};

char *split(const char *path);
struct dir_listing *create_dirlisting(const char *path);
void destroy_dirlisting(struct dir_listing **dirents);

int is_directory(const char *path);
int is_file(const char *path);
size_t get_file_size(const char *path);
char *get_file_contents(const char *path);
