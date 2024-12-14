/**
 * FUSE implemementation of a virtual filesystem
 * querying data for the UIUC course explorer
 * mirrors all calls onto a backing filesystem
 * with temporal information saved to a file
 */

#define FUSE_USE_VERSION 31

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse3/fuse.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// =============================================================================
// OPTIONS =====================================================================
// =============================================================================

// TODO: configuration
// -- expiration time of cache
// -- root directory
// -- metadata file

#define DIR_EXE ("cs341_course_explorer_api.py")
#define METADATA_FILE "__metadata.dat"
#define ROOT "/.local/share/cs341_fs/"

// the time (seconds) since the file is seen as stale
#define EXPR_TIME (120)

static char *HOME = NULL;

// every directory has a metafile "dir/__metadata.dat" that stores the contents
// the format is then going to be
// 1) the number of files / directories inside
// 2) the time (double) that it was last fetched -- consider using modify time of file
// 3) an array of metadata items -- with pointer to where the variable length starts
// 4) the variable length file contents
struct metadata
{
    char filepath[FILENAME_MAX];
    size_t contents_size;
    size_t contents_offset;
    int is_directory;
    int is_file;
};

// =============================================================================
// PRIVATE HELPERS =============================================================
// =============================================================================

/* changes it to absolute paths */
char *normalize_path(const char *path)
{
    return (*path == '.') ? strdup("/") : strdup(path);
}

/* duplicates a new string for the parent directory of a path */
char *get_parent_dir(const char *path)
{
    char *p = strdup(path);
    char *s = strrchr(p, '/');

    if (s == NULL)
        return NULL;

    *s = '\0';
    return p;
}

static double get_time(void)
{
    struct timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    return ((double)res.tv_sec * 1e9 + (double)res.tv_nsec) / 1e9;
}

// TODO: make these take flags for both read and write

/**
 * returns the fd referring to the metadata for a given
 * normalized path or dir
 */
int open_metadata_file(const char *dir)
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

/**
 * writes all data of a path into the mmap-ed file descriptor at fd
 */
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

    waitpid(child, NULL, 0);
    return 0;
}

/**
 * maps the metadafile corresponding to a directory
 * fills the length and assigns the fd
 */
char *map_metadata(const char *path, size_t *length, int *fd)
{
    if (*path == '/')
    {
        char *p = strdup(path + 1);
        strcat(p, "/");
        *fd = open_metadata_file(p);
        free(p);

        fill_directory_contents(path + 1, *fd);
    }
    else
    {
        *fd = open_metadata_file(path);
        fill_directory_contents(path, *fd);
    }

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

// =============================================================================
// PUBLIC INTERFACE SYSCALLS  ==================================================
// =============================================================================

static void *my_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    (void)conn;
    (void)cfg;

    return NULL;
}

#define goclean(a) \
    retval = (a);  \
    goto cleanup;

static int my_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    (void)fi;
    memset(stbuf, 0, sizeof(struct stat));

    char *mypath = normalize_path(path);
    char *parent = get_parent_dir(mypath);

    // map relative paths to absolute
    int fd, retval;
    size_t size;
    char *data = map_metadata(parent, &size, &fd);

    if (data == NULL)
        return -errno;

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

static int my_readdir(
    const char *path,
    void *buf,
    fuse_fill_dir_t filler,
    off_t offset,
    struct fuse_file_info *fi,
    enum fuse_readdir_flags flags)
{
    (void)offset;
    (void)fi;
    (void)flags;

    filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
    filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);

    int fd;
    size_t size;
    char *mypath = normalize_path(path);
    char *data;
    if (strlen(mypath) == 1)
        data = map_metadata("", &size, &fd);
    else
        data = map_metadata(mypath, &size, &fd);

    if (data == NULL)
        return -errno;

    size_t num_items, item;
    memcpy(&num_items, data, sizeof(size_t));

    struct metadata *metas = (struct metadata *)(data + sizeof(size_t) + sizeof(double));
    for (item = 0; item < num_items; ++item)
        filler(buf, metas[item].filepath, NULL, 0, FUSE_FILL_DIR_PLUS);

    close(fd);
    munmap(data, size);
    return 0;
}

static int my_open(const char *path, struct fuse_file_info *fi)
{
    struct stat stbuf;
    my_getattr(path, &stbuf, fi);

    if (!S_ISREG(stbuf.st_mode))
        return -ENOENT;

    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;

    return 0;
}

static int my_read(
    const char *path,
    char *buf,
    size_t size,
    off_t offset,
    struct fuse_file_info *fi)
{
    int res = my_open(path, fi);
    if (res != 0)
        return res;

    char *mypath = normalize_path(path);
    char *parent = get_parent_dir(mypath);

    int fd, retval;
    size_t length;
    char *data = map_metadata(parent, &length, &fd);

    if (data == NULL)
        return -errno;

    char *bottom = strrchr(mypath, '/') + 1;

    size_t num_items, item;
    memcpy(&num_items, data, sizeof(size_t));

    struct metadata *metas = (struct metadata *)(data + sizeof(size_t) + sizeof(double));
    for (item = 0; item < num_items; ++item)
    {
        struct metadata *meta = metas + item;
        if (strcmp(meta->filepath, bottom) == 0)
        {
            char *contents = data + meta->contents_offset + offset;
            memcpy(buf, contents, size);
            goclean(size);
        }
    }
    goclean(-ENOENT);

cleanup:
    close(fd);
    munmap(data, length);
    return retval;
}

static const struct fuse_operations my_oper = {
    .init = my_init,       // init
    .getattr = my_getattr, // stat
    .open = my_open,       // open
    .readdir = my_readdir, // readdir
    .read = my_read,       // read
};

// =============================================================================
// MAIN HOOK  ==================================================================
// =============================================================================

/* Command line options */
static struct options
{
    int show_help;
} options;

#define OPTION(t, p) {t, offsetof(struct options, p), 1}
static const struct fuse_opt option_spec[] = {
    OPTION("-h", show_help),
    OPTION("--help", show_help),
    FUSE_OPT_END,
};

static void show_help(const char *progname)
{
    printf("usage: %s [options] <mountpoint>\n\n", progname);
}

int main(int argc, char *argv[])
{
    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    HOME = getenv("HOME");
    if (HOME == NULL)
        HOME = getpwuid(getuid())->pw_dir;

    /* Parse options */
    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
        return 1;

    if (options.show_help)
    {
        show_help(argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    }

    ret = fuse_main(args.argc, args.argv, &my_oper, NULL);
    fuse_opt_free_args(&args);
    return ret;
}
