#define FUSE_USE_VERSION 31

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse3/fuse.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "vector.h"

// this needs to be added to path :)
#define DIR_EXE ("api.py")
#define min(a, b) (((a) < (b)) ? (a) : (b))

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

struct dir_listing
{
    /**
     * vector of char *
     * each directory entry is going to be a canonical path
     */
    vector *dirents;
};

/**
 * space delimits a given path to prepare for cmd
 * a/b/c/d -> a b c d
 */
static char *split(const char *path)
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

struct dir_listing *create_dirlisting(const char *path)
{
    struct dir_listing *dirents = malloc(sizeof(struct dir_listing));
    dirents->dirents = string_vector_create();

    char *path_split = split(path);
    char *cmd = malloc(strlen(path_split) + strlen("-s") + strlen(DIR_EXE) + 3);
    sprintf(cmd, "%s -s %s", DIR_EXE, path_split);

    FILE *f = popen(cmd, "r");
    assert(f);

    char line[256];
    while (fgets(line, sizeof(line), f))
        vector_push_back(dirents->dirents, line);

    pclose(f);
    return dirents;
}

void destroy_dirlisting(struct dir_listing **dirents)
{
    assert(dirents && *dirents);

    vector_destroy((*dirents)->dirents);
    free(*dirents);
    *dirents = NULL;
}

static int is_directory(const char *path)
{
    if (strcmp(path, "") == 0)
        return 1;

    char *cmd = malloc(strlen(DIR_EXE) + strlen("-d") + strlen(path) + 3);
    sprintf(cmd, "%s -d %s", DIR_EXE, path);

    FILE *f = popen(cmd, "r");
    assert(f);

    char res = '0';
    fread(&res, 1, 1, f);

    pclose(f);
    return res == '1';
}

static int is_file(const char *path)
{
    if (strcmp(path, "") == 0)
        return 0;

    char *cmd = malloc(strlen(DIR_EXE) + strlen("-f") + strlen(path) + 3);
    sprintf(cmd, "%s -f %s", DIR_EXE, path);

    FILE *f = popen(cmd, "r");
    assert(f);

    char res = '0';
    fread(&res, 1, 1, f);

    pclose(f);
    return res == '1';
}

size_t get_file_size(const char *path)
{
    char *path_split = split(path);
    char *cmd = malloc(strlen(path_split) + strlen("-s") + strlen(DIR_EXE) + 3);
    sprintf(cmd, "%s -s %s", DIR_EXE, path_split);

    FILE *f = popen(cmd, "r");
    assert(f);

    char buf[256] = {0};
    fread(buf, 256, 256, f);

    free(path_split);
    free(cmd);
    pclose(f);

    return strlen(buf);
}

char *get_file_contents(const char *path)
{
    if (!is_file(path))
        return NULL;

    char *path_split = split(path);
    char *cmd = malloc(strlen(path_split) + strlen("-s") + strlen(DIR_EXE) + 3);
    sprintf(cmd, "%s -s %s", DIR_EXE, path_split);

    FILE *f = popen(cmd, "r");
    assert(f);

    char *line = malloc(256);
    fgets(line, 256, f);
    pclose(f);

    return line;
}

static void *hello_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    (void)conn;
    cfg->kernel_cache = 1;
    return NULL;
}

static int hello_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    (void)fi;

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    int isd = is_directory(path + 1);
    int isf = is_file(path + 1);

    if (isd)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    if (isf)
    {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = get_file_size(path);
        return 0;
    }

    return -ENOENT;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi,
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
    });

    destroy_dirlisting(&dir_listing);
    return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
    int isf = is_file(path + 1);
    if (!isf)
        return -ENOENT;

    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;

    return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int open_status = hello_open(path, fi);
    if (open_status < 0)
        return open_status;

    char *path_split = split(path);
    char *cmd = malloc(strlen(path_split) + strlen("-s") + strlen(DIR_EXE) + 3);
    sprintf(cmd, "%s -s %s", DIR_EXE, path_split);

    FILE *f = popen(cmd, "r");
    assert(f);

    char contents[256] = {0};
    fread(contents, 1, 256, f);
    size_t bytes = strlen(contents);

    if (offset < (ssize_t)bytes)
    {
        size_t to_read = min(size, bytes - offset);
        memcpy(buf, contents + offset, to_read);
    }

    free(path_split);
    free(cmd);
    pclose(f);

    return strlen(buf);
}

static const struct fuse_operations hello_oper = {
    .init = hello_init,
    .getattr = hello_getattr,
    .readdir = hello_readdir,
    .open = hello_open,
    .read = hello_read,
};

static void show_help(const char *progname)
{
    printf("usage: %s [options] <mountpoint>\n\n", progname);
}

int main(int argc, char *argv[])
{
    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    /* Set defaults -- we have to use strdup so that
       fuse_opt_parse can free the defaults if other
       values are specified */
    // options.filename = strdup("hello");
    // options.contents = strdup("Hello World!\n");

    /* Parse options */
    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
        return 1;

    /* When --help is specified, first print our own file-system
       specific help text, then signal fuse_main to show
       additional help (by adding `--help` to the options again)
       without usage: line (by setting argv[0] to the empty
       string) */
    if (options.show_help)
    {
        show_help(argv[0]);
        assert(fuse_opt_add_arg(&args, "--help") == 0);
        args.argv[0][0] = '\0';
    }

    ret = fuse_main(args.argc, args.argv, &hello_oper, NULL);
    fuse_opt_free_args(&args);
    return ret;
}
