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

    // map relative paths to absolute
    int fd, retval;
    size_t size;
    char *data = map_metadata(HOME, path, &size, &fd);

    if (data == NULL)
    {
        close(fd);
        return -errno;
    }

    char *mypath = normalize_path(path);
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
    // TODO:
    return 0;
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
