#include <assert.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "vector.h"
#include "client.h"

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

int is_directory(const char *path)
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

int is_file(const char *path)
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
    if (!is_file(split(path)))
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
