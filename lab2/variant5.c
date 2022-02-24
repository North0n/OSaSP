#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int printDirInfo(const char *dirname)
{
    DIR *dir;
    if ((dir = opendir(dirname)) == NULL) {
        perror("Error during attempt to open directory.\n");
        return -1;
    }
    struct dirent *dirent;
    struct stat stats;
    int size = 0;
    int filesCount = 0;
    int maxSize = 0;
    char *maxSizeFilename = (char *)malloc((NAME_MAX + 1) * sizeof(char));
    char *temp = NULL;
    while ((dirent = readdir(dir)) != NULL) {
        if (strcmp(dirname, ".") ^ strcmp(dirname, "..")) // Мб сделать не xor а по-нормальному
            return 0;
        if (stat(dirent->d_name, &stats)) { // Мб заменить на lstat
            printf("Path: %s Filename: %s", getcwd(temp, 0), dirent->d_name);
            perror("Error during attempt to get info about file.\n");
            return -2; // Добавить закрытие файлов и возможность выбрасывание ошибки вверх в рекурсии (а мб и не добавлять)
        }
        if (S_ISDIR(stats.st_mode))
            printDirInfo(dirent->d_name);
        else {
            size += stats.st_size;
            ++filesCount;
            if (stats.st_size > maxSize) {
                strcpy(maxSizeFilename, dirent->d_name);
                maxSize = stats.st_size;
            }
        }
    }
    printf("Name: %s Size: %d File's count: %d File with max size: %s\n", dirname, size, filesCount, maxSizeFilename);
    if (closedir(dir)) {
        perror("Error during attempt to close directory.\n");
        return -3;
    }
    return 0;
}

int printDir(const char *dirname)
{
    DIR *dir;
    if ((dir = opendir(dirname)) == NULL) {
        printf("Error during attempt to open directory %s\n", dirname);
        return 1;
    }
    printf("Contents of %s:\n", dirname);
    struct dirent *dirent;
    struct stat stats;
    while ((dirent = readdir(dir)) != NULL) {
        stat(dirent->d_name, &stats); // Добавить проверку на появление ошибки
        if (S_ISDIR(stats.st_mode))
            printf("    Name: %s Size: %ld Dir: %s\n", dirent->d_name, stats.st_size, S_ISDIR(stats.st_mode) ? "true" : "false");
        else
            printf("    Name: %s\n", dirent->d_name);
    }
    if (closedir(dir)) {
        printf("Error during attempt to close directory %s\n", dirname);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("You should enter 2 parameters:\n");
        printf("    First - directory's name\n");
        printf("    Second - output file's name\n");
        return 1;
    }

    int errorCode = printDirInfo(argv[1]);

    return errorCode;
}