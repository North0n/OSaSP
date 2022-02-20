#include <stdio.h>
#include <dirent.h>

int printDir(const char *dirname)
{
    DIR *dir;
    printf("Contents of %s:\n", dirname);
    if ((dir = opendir(dirname)) == NULL) {
        printf("Error during attempt to open directory %s\n", dirname);
        return 1;
    }
    struct dirent *dirent;
    while ((dirent = readdir(dir)) != NULL) {
        printf("    %s\n", dirent->d_name);
    }
    if (closedir(dir)) {
        printf("Error during attempt to close directory %s\n", dirname);
        return 1;
    }

    return 0;
}

int main()
{
    int errorCode = 0;
    errorCode |= printDir("./");
    errorCode |= printDir("/");

    return errorCode;
}