#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>

// #define DIR_EXE ("course_explorer_hardcode.py")
#define DIR_EXE ("cs341_course_explorer_api.py")

// replaces all '/' in a path with ' '
char *split(const char *path);

char *get_parent_dir(const char *path);

// replaces with absolute path
char *normalize_path(const char *path);

// writes it into the file specified at fd
int fill_directory_contents(const char *path, int fd);
/**
 * does the memory mapping of the metadata file
 * also filles the contents (if necessary)
 */
char *map_metadata(const char *HOME, const char *path, size_t *length, int *fd);
