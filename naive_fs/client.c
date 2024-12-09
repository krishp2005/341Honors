#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "client.h"

#define METADATA_FILE "__metadata.dat"
#define ROOT "/.local/share/cs341_fs/"

// the time (seconds) since the file is seen as stale
#define EXPR_TIME (15)

/* changes it to absolute paths */
char *normalize_path(const char *path)
{
    return (*path == '.') ? strdup("/") : strdup(path);
}

char *get_parent_dir(const char *path)
{
    char *p = strdup(path);
    char *s = strrchr(p, '/');

    if (s == NULL)
        return NULL;

    *s = '\0';
    return p;
}

// TOOD: make these take flags for both read and write

/**
 * returns the fd referring to the metadata for a given
 * normalized path or dir
 */
int open_metadata_file(const char *HOME, const char *dir)
{
    char buf[FILENAME_MAX] = {0};
    sprintf(buf, "%s%s%s" METADATA_FILE, HOME, ROOT, dir);
    return open(buf, O_CREAT | O_RDWR, 0644);
}

char *map_metadata(const char *HOME, const char *path, size_t *length, int *fd)
{
    char *mypath = normalize_path(path);
    char *parent = get_parent_dir(mypath);

    *fd = open_metadata_file(HOME, mypath);
    fill_directory_contents(parent, *fd);

    struct stat mystbuf;
    fstat(*fd, &mystbuf);
    *length = mystbuf.st_size;

    char *data = mmap(NULL, *length, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
    free(mypath);
    free(parent);

    if (data == MAP_FAILED)
    {
        close(*fd);
        return NULL;
    }

    return data;
}

/**
 * space delimits a given path to prepare for cmd
 * a/b/c/d -> a b c d
 */
char *split(const char *path)
{
    char *ret = strdup(path);
    char *iter = ret;
    while (iter)
    {
        iter = strchr(iter, '/');
        if (iter != NULL)
            *iter = ' ';
    }

    return ret;
}

static double get_time(void)
{
    struct timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    return ((double)res.tv_sec * 1e9 + (double)res.tv_nsec) / 1e9;
}

// writes it into the file specified at fd
int fill_directory_contents(const char *path, int fd)
{
    lseek(fd, sizeof(size_t), SEEK_SET);
    double buf;
    read(fd, &buf, sizeof(double));
    if (get_time() - buf <= EXPR_TIME)
        return 0;

    fflush(stdout);
    pid_t child = fork();
    if (child == -1)
        return errno;

    if (child == 0)
    {
        // we are the child
        lseek(fd, 0, SEEK_SET);
        dup2(fd, stdout);
        execlp(DIR_EXE, DIR_EXE, path, NULL);
        exit(1);
    }

    waitpid(child, NULL, 0);
    return 0;
}
