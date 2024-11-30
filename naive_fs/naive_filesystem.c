/**
 * simplified version of finding filesystems
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
#include <unistd.h>

#include "client.h"

// =============================================================================
// OPTIONS =====================================================================
// =============================================================================

// TODO: configuration
// -- expiration time of cache
// -- root file
// -- metadata file

#define METADATA_FILE "__metadata.dat"
#define ROOT "/.local/share/cs341_fs/"

static char *HOME = NULL;
#define NEWPATH(path, newpath)                             \
    char _newpath[FILENAME_MAX + 1];                       \
    sprintf(_newpath, "%s%s%s", HOME, ROOT, path);         \
    if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0) \
        sprintf(_newpath, "%s%s", HOME, ROOT);             \
    char *(newpath) = _newpath;

// the metadata file is represented as an array of NUM_INODES doubles
// double represents the unix time (in seconds since 1970)
// 0 (default) means uninitialized
// this is the time since the information was last successfully fetched
// note: this is different from access time
// potentially, this could be the utimens call
#define NUM_INODES (1024L * 1024L)

static double get_time(void)
{
    struct timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    return ((double)res.tv_sec * 1e9 + (double)res.tv_nsec) / 1e9;
}

// the time (seconds) since the file is seen as stale
#define EXPR_TIME (15)

// static pointer to mmaped file
static char *metadata;

// =============================================================================
// PRIVATE INTERFACE SYSCALLS  =================================================
// =============================================================================

// static int my_mknod(const char *path, mode_t mode, dev_t rdev)
// {
//     int res;
//     int dirfd = AT_FDCWD;
//     char *link = NULL;
//
//     NEWPATH(path, newpath);
//     if (S_ISREG(mode))
//     {
//         res = openat(dirfd, newpath, O_CREAT | O_EXCL | O_WRONLY, mode);
//         if (res >= 0)
//             res = close(res);
//     }
//     else if (S_ISDIR(mode))
//         res = mkdirat(dirfd, newpath, mode);
//     else if (S_ISLNK(mode) && link != NULL)
//         res = symlinkat(link, dirfd, newpath);
//     else if (S_ISFIFO(mode))
//         res = mkfifoat(dirfd, newpath, mode);
//     else
//         res = mknodat(dirfd, newpath, mode, rdev);
//
//     if (res == -1)
//         return -errno;
//     return res;
// }
//
// int my_mkdir(const char *path, mode_t mode)
// {
//     NEWPATH(path, newpath);
//
//     int res = mkdir(newpath, mode);
//     return (res == -1) ? -errno : res;
// }
//
// int my_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
// {
//     int fd;
//     int res;
//
//     if (fi == NULL)
//         fd = open(path, O_WRONLY);
//     else
//         fd = (int)fi->fh;
//
//     if (fd == -1)
//         return -errno;
//
//     res = (int)pwrite(fd, buf, size, offset);
//     if (res == -1)
//         res = -errno;
//
//     if (fi == NULL)
//         close(fd);
//     return res;
// }

// =============================================================================
// PUBLIC INTERFACE SYSCALLS  ==================================================
// =============================================================================

static void *my_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    (void)conn;
    (void)cfg;

    // create the metadata file
    // NEWPATH(METADATA_FILE, path);
    // int fd = open(path, O_TRUNC | O_CREAT | O_RDWR, 0666);
    // metadata = mmap(NULL, NUM_INODES * sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    return NULL;
}

static int my_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    (void)fi;

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    // TODO: merge these three calls into one bigger one

    if (is_directory(split(path + 1)))
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    if (is_file(split(path + 1)))
    {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = (off_t)get_file_size(path);
        return 0;
    }

    return -ENOENT;
}

static int my_open(const char *path, struct fuse_file_info *fi)
{
    int isf = is_file(split(path + 1));
    if (!isf)
        return -ENOENT;

    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;

    return 0;
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

    struct dir_listing *dir_listing = create_dirlisting(path + 1);
    VECTOR_FOR_EACH(dir_listing->dirents, _dirent, {
        char *dirent = _dirent;
        dirent[strlen(dirent) - 1] = '\0';
        filler(buf, dirent, NULL, 0, FUSE_FILL_DIR_PLUS);

        char buf[FILENAME_MAX];
        sprintf(buf, "%s %s", split(path + 1), dirent);
        int isd = is_directory(buf);
        int isf = is_file(buf);

        // TODO: use myopen

        sprintf(buf, "%s%s%s/%s", HOME, ROOT, path + 1, dirent);
        if (isf)
        {
            int fd = open(buf, O_CREAT | O_TRUNC | O_RDWR, 0644);
            close(fd);
        }
        else if (isd)
        {
            int fd = mkdir(buf, 0755);
            close(fd);
        }
    });

    destroy_dirlisting(&dir_listing);
    return 0;
}

static int my_read(
    const char *path,
    char *buf,
    size_t size,
    off_t offset,
    struct fuse_file_info *fi)
{
    int fd;
    ssize_t res;

    if (fi == NULL)
        fd = open(path, O_RDONLY);
    else
        fd = (int)fi->fh;

    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    if (fi == NULL)
        close(fd);
    return (int)res;
}

static const struct fuse_operations my_oper = {
    .init = my_init,       // init
    .getattr = my_getattr, // stat
    .open = my_open,       // open
    .readdir = my_readdir, // readdir
    .read = my_read,       // read
    // .mknod = my_mknod,     // create inode
    // .write = my_write,     // write
    // .utimens = my_utimens, // modify time
    // .mkdir = my_mkdir,     // mkdir
    // .unlink = my_unlink,   // rm
    // .rmdir = my_rmdir,     // rmdir
};

// =============================================================================
// MAIN HOOK  ==================================================================
// =============================================================================

// FIXME: integrate this with the mess of options we have above

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
