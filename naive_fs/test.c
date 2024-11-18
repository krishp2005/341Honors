#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

void dirlist(char *path)
{
    struct stat s;

    struct dirent *dp;
    DIR *dirp = opendir(path);
    if (dirp == NULL)
    {
        perror("Couldnt open path");
        return;
    }

    while ((dp = readdir(dirp)) != NULL)
    {
        if (strcmp(".", dp->d_name) == 0 || strcmp("..", dp->d_name) == 00)
            continue;

        char newpath[strlen(path) + strlen(dp->d_name) + 2];

        sprintf(newpath, "%s/%s", path, dp->d_name);

        printf("%s\n", newpath);
        // recurse only if it is not a symbolic link
        // need to use lstat not stat
        if (lstat(newpath, &s) == 0 && S_ISDIR(s.st_mode) && !S_ISLNK(s.st_mode))
            dirlist(newpath);
    }
    closedir(dirp);
}

int main(void)
{
    char *path = "~/.local/share/cs341_fs/";
    dirlist(path);

    // int fd = open("mirror_fs/test", O_CREAT | O_TRUNC | O_RDWR, 0666);
    // assert(fd);
    // close(fd);
}
