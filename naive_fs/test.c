#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
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

char *get_parent_dir(const char *path)
{
    char *p = strdup(path);
    char *s = strrchr(p, '/');

    if (s == NULL)
        return NULL;

    *s = '\0';
    return p;
}

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

char *normalize_path(const char *path)
{
    return (*path == '.') ? strdup("/") : strdup(path);
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

        int idx = 1 + (strlen(iter) > 0);
        if (idx > 1)
            argv[idx - 1] = iter;
        while (strchr(iter, '/') != NULL)
        {
            iter = strchr(iter, '/');
            *iter++ = '\0';
            argv[idx++] = iter;
        }

        dup2(fd, STDOUT_FILENO);
        execvp(DIR_EXE, argv);
        exit(1);
    }

    return 0;
}

#define HOME "/home/ianchen3"
#define goclean(a) \
    retval = (a);  \
    goto cleanup;

struct metadata
{
    char filepath[FILENAME_MAX];
    size_t contents_size;
    size_t contents_offset;
    int is_directory;
    int is_file;
};

static int my_getattr(const char *path, struct stat *stbuf)
{
    memset(stbuf, 0, sizeof(struct stat));

    char *mypath = normalize_path(path);
    char *parent = get_parent_dir(mypath);

    // map relative paths to absolute
    int fd, retval;
    size_t size;
    char *data = map_metadata(HOME, parent, &size, &fd);

    if (data == NULL)
    {
        close(fd);
        return -errno;
    }

    if (strcmp(mypath, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        goclean(0);
    }

    char *bottom = strrchr(mypath, '/') + 1;

    size_t num_items, item;
    memcpy(&num_items, data, sizeof(size_t));
    struct metadata *metas = (struct metadata *)(data + sizeof(size_t) + sizeof(double));
    for (item = 0; item < num_items; ++item)
    {
        struct metadata *meta = &metas[item];
        if (strcmp(meta->filepath, bottom) == 0)
        {
            if (meta->is_directory)
            {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
            }
            else if (meta->is_file)
            {
                stbuf->st_mode = S_IFREG | 0644;
                stbuf->st_nlink = 1;
                stbuf->st_size = (off_t)meta->contents_size;
            }
            else
                break;

            goclean(0);
        }
    }

    goclean(-ENOENT);

cleanup:
    close(fd);
    munmap(data, size);
    free(mypath);
    return retval;
}

static int my_readdir(const char *path)
{
    int fd;
    size_t size;
    char *data = map_metadata(HOME, path, &size, &fd);

    if (data == NULL)
    {
        close(fd);
        return -errno;
    }

    size_t num_items, item;
    memcpy(&num_items, data, sizeof(size_t));

    struct metadata *metas = (struct metadata *)(data + sizeof(size_t) + sizeof(double));
    for (item = 0; item < num_items; ++item)
        printf("%s\n", metas[item].filepath);

    close(fd);
    munmap(data, size);
    return 0;
}

int main(int argc, char **argv)
{
    // char *path = "~/.local/share/cs341_fs/";
    // char *path = "/2024";
    // size_t length;
    // int fd;
    // char *meta = map_metadata(HOME, path, &length, &fd);

    // size_t num_items;
    // double time;
    // memcpy(&num_items, meta, sizeof(num_items));
    // memcpy(&time, meta + sizeof(num_items), sizeof(time));

    // munmap(meta, length);
    // close(fd);

    // struct stat stbuf;
    // int res = my_getattr("/2024", &stbuf);
    // printf("stat returned %d\n", res);

    if (argc == 1)
    {
        printf("run with args\n");
        exit(0);
    }

    int res = my_readdir(argv[1]);
    printf("readdir exited with res %d\n", res);

    // int fd = open("mirror_fs/test", O_CREAT | O_TRUNC | O_RDWR, 0666);
    // assert(fd);
    // close(fd);
}
