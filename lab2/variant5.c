#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define FORMAT_STR1 "%100s %8d %5d "
#define FORMAT_STR2 "%50s"
#define FORMAT_STR_HEADER "%100s %8s %5s %50s\n"

int printDirInfo(char *dirname, FILE *file)
{
    DIR *dir;
    if ((dir = opendir(dirname)) == NULL) {
        perror("Error during attempt to open directory.\n");
        return -2;
    }
    struct dirent *dirent;
    struct stat stats;
    int size = 0;
    int filesCount = 0;
    int maxSize = 0;
    char *maxSizeFilename = (char *)malloc((NAME_MAX + 1) * sizeof(char));
    maxSizeFilename[0] = '\0';
    char *temp = NULL;
    int length = strlen(dirname);
    int error = 0;
    while ((dirent = readdir(dir)) != NULL) {
        if (strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0)
            continue;
        dirname[length] = '/';
        dirname[length + 1] = '\0';
        strcat(dirname, dirent->d_name);
        if (lstat(dirname, &stats)) { 
            fprintf(stderr, "Filename: %s\n", dirname);
            perror("Error during attempt to get info about file.\n");
            continue;
        }
        if (S_ISDIR(stats.st_mode))
            error |= printDirInfo(dirname, file);
        else {
            size += stats.st_size;
            ++filesCount;
            if (stats.st_size > maxSize) {
                strcpy(maxSizeFilename, dirent->d_name);
                maxSize = stats.st_size;
            }
        }
    }
    dirname[length] = '/';
    dirname[length + 1] = '\0';
    printf(FORMAT_STR1, dirname, size, filesCount);
    fprintf(file, FORMAT_STR1, dirname, size, filesCount);
    if (filesCount != 0) {
        printf(FORMAT_STR2, maxSizeFilename);
        fprintf(file, FORMAT_STR2, maxSizeFilename);
    }
    printf("\n");
    fprintf(file, "\n");
    if (closedir(dir)) {
        perror("Error during attempt to close directory.\n");
        return -4;
    }
    return error;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("You should enter 2 parameters:\n");
        printf("    First - directory's name\n");
        printf("    Second - output file's name\n");
        return -1;
    }

    char *buf = (char*)malloc(PATH_MAX * sizeof(char));
    strcpy(buf, argv[1]);

    FILE *outputFile;
    if ((outputFile = fopen(argv[2], "w")) == NULL)
        perror("Error during attempt to open output file.\n");
    fprintf(outputFile, FORMAT_STR_HEADER, "Filename", "Size", "Count", "Max size");
    printf(FORMAT_STR_HEADER, "Filename", "Size", "Count", "Max size");
    int errorCode = printDirInfo(buf, outputFile);
    if (fclose(outputFile)) 
        perror("Erorr during attempt to close output file. May cause loss of data.\n");

    return errorCode;
}