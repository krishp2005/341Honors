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

    char *parent = get_parent_dir(buf);
    char command[FILENAME_MAX];
    snprintf(command, sizeof(command), "/bin/env mkdir -p %s", parent);
    system(command);
    free(parent);

    return open(buf, O_CREAT | O_RDWR, 0644);
}

char *map_metadata(const char *HOME, const char *path, size_t *length, int *fd)
{
    if (*path == '/')
    {
        char *p = strdup(path + 1);
        strcat(p, "/");
        *fd = open_metadata_file(HOME, p);
        free(p);
    }
    else
        *fd = open_metadata_file(HOME, path);

    fill_directory_contents(path + 1, *fd);

    struct stat mystbuf;
    fstat(*fd, &mystbuf);
    *length = mystbuf.st_size;

    char *data = mmap(NULL, *length, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);

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
    if (read(fd, &buf, sizeof(double)) == sizeof(double) && get_time() - buf <= EXPR_TIME)
        return 0;

    lseek(fd, 0, SEEK_SET);
    ftruncate(fd, 0);

    fflush(stdout);
    int child = fork();
    if (child == -1)
        return -errno;

    if (child == 0)
    {
        char **argv = (char **)calloc(11, sizeof(char *));
        char *iter = strdup(path);
        argv[0] = DIR_EXE;
        argv[1] = iter;
        int idx = 1;
        while (strchr(iter, '/') != NULL)
        {
            iter = strchr(iter, '/');
            *iter++ = '\0';
            argv[++idx] = iter;
        }

        dup2(fd, STDOUT_FILENO);
        execvp(DIR_EXE, argv);
        exit(1);
    }

    return 0;
}
