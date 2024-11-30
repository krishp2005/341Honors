// HEAVILY inspired by https://github.com/libfuse/libfuse/blob/master/example/passthrough.c

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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

static char *ROOT = "/.local/share/cs341_fs";
static char *HOME = NULL;

#define NEWPATH(path)                                      \
    char newpath[FILENAME_MAX + 1];                        \
    sprintf(newpath, "%s%s%s", HOME, ROOT, path);          \
    if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0) \
        sprintf(newpath, "%s%s", HOME, ROOT);

/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
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

static void *my_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    (void)conn;
    cfg->kernel_cache = 1;
    return NULL;
}

static int my_getattr(const char *path, struct stat *stbuf,
                      struct fuse_file_info *fi)
{
    (void)fi;

    memset(stbuf, 0, sizeof(struct stat));

    NEWPATH(path);
    int res = lstat(newpath, stbuf);
    return (res == -1) ? -errno : res;
}

static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi,
                      enum fuse_readdir_flags flags)
{
    (void)offset;
    (void)fi;
    (void)flags;

    NEWPATH(path);
    DIR *dp = opendir(newpath);
    if (dp == NULL)
        return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL)
    {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0, FUSE_FILL_DIR_PLUS))
            break;
    }

    closedir(dp);
    return 0;
}

static int my_open(const char *path, struct fuse_file_info *fi)
{
    NEWPATH(path);
    int res = open(newpath, fi->flags);
    if (res == -1)
        return -errno;
    fi->fh = res;
    return 0;
}

static int my_read(const char *path, char *buf, size_t size, off_t offset,
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

int my_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int fd;
    int res;

    if (fi == NULL)
        fd = open(path, O_WRONLY);
    else
        fd = (int)fi->fh;

    if (fd == -1)
        return -errno;

    res = (int)pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    if (fi == NULL)
        close(fd);
    return res;
}

static int my_mknod(const char *path, mode_t mode, dev_t rdev)
{
    int res;
    int dirfd = AT_FDCWD;
    char *link = NULL;

    NEWPATH(path);
    if (S_ISREG(mode))
    {
        res = openat(dirfd, newpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (res >= 0)
            res = close(res);
    }
    else if (S_ISDIR(mode))
        res = mkdirat(dirfd, newpath, mode);
    else if (S_ISLNK(mode) && link != NULL)
        res = symlinkat(link, dirfd, newpath);
    else if (S_ISFIFO(mode))
        res = mkfifoat(dirfd, newpath, mode);
    else
        res = mknodat(dirfd, newpath, mode, rdev);

    if (res == -1)
        return -errno;
    return res;
}

int my_mkdir(const char *path, mode_t mode)
{
    NEWPATH(path);

    int res = mkdir(newpath, mode);
    return (res == -1) ? -errno : res;
}

int my_rmdir(const char *path)
{
    NEWPATH(path);

    int res = rmdir(newpath);
    return (res == -1) ? -errno : res;
}

static int my_utimens(const char *path, const struct timespec ts[2], struct fuse_file_info *fi)
{
    (void)fi;
    NEWPATH(path);
    int res = utimensat(0, newpath, ts, AT_SYMLINK_NOFOLLOW);
    if (res == -1)
        return -errno;

    return 0;
}

static int my_unlink(const char *path)
{
    int res;

    NEWPATH(path);
    res = unlink(newpath);
    if (res == -1)

        return -errno;

    return 0;
}

static const struct fuse_operations my_oper = {
    .init = my_init,       // init
    .getattr = my_getattr, // stat
    .mknod = my_mknod,     // create inode
    .open = my_open,       // open
    .read = my_read,       // read
    .write = my_write,     // write
    .utimens = my_utimens, // modify time
    .readdir = my_readdir, // readdir
    .mkdir = my_mkdir,     // mkdir
    .unlink = my_unlink,   // rm
    .rmdir = my_rmdir,     // rmdir
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
