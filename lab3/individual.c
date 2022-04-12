#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define ERR_FILE_OPEN   (-2)
#define ERR_FILE_CLOSE  (-3)
#define ERR_DIR_OPEN    (-4)
#define ERR_DIR_CLOSE   (-5)
#define ERR_WAIT        (-6)
#define ERR_ARGS_COUNT  (-1)
#define ERR_ARGS_NUM    (-7)

long processesCount = 1;
long maxProcessesCount = 0;

int findFreq(const char *filename, const char *substr, int *lettersCount)
{
    FILE *file;
    if ((file = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Filename: %s \n", filename);
        perror("Error occurred during attempt to open file\n");
        return ERR_FILE_OPEN;
    }

    int ch;
    unsigned long index = 0;
    int frequency = 0;
    unsigned long substrLen = strlen(substr);
    *lettersCount = 0;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == substr[index]) {
            index = (index + 1) % substrLen;
            if (index == 0)
                ++frequency;
        } else {
            index = 0;
        }
        ++(*lettersCount);
    }

    if (fclose(file) == EOF) {
        perror("Error occurred during attempt to close file, may cause loss of data");
        return ERR_FILE_CLOSE;
    }

    return frequency;
}

int dirTraversal(char *filename, const char *substr)
{
    DIR *dir;
    if ((dir = opendir(filename)) == NULL) {
        perror("Error during attempt to open directory.\n");
        return ERR_DIR_OPEN;
    }

    struct dirent* dirEntry;
    int error = 0;
    struct stat stats;
    int frequency, bytesCount;
    int length = (int)strlen(filename);

    while ((dirEntry = readdir(dir)) != NULL) {
        // Skip current and previous directory
        if (strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0)
            continue;

        // Form current file filename
        filename[length] = '/';
        filename[length + 1] = '\0';
        strcat(filename, dirEntry->d_name);
        if (lstat(filename, &stats) != 0) {
            perror("Error during attempt to get info about file.\n");
            fprintf(stderr, "Filename: %s\n", filename);
            continue;
        }

        // Current file processing
        if (S_ISDIR(stats.st_mode))
            error |= dirTraversal(filename, substr);
        else if (S_ISREG(stats.st_mode)) {
            // Ensures that process count is less than or equal to maximum process count
            if (processesCount >= maxProcessesCount) {
                if (wait(NULL) == -1) {
                    perror("Error during attempt to wait process termination\n");
                    --processesCount;
                    return ERR_WAIT;
                }
                --processesCount;
            }
            // Creation of a new process for file handling
            switch (fork()) {
                case -1:
                    perror("Error during attempt to start another process\n");
                    continue;
                case 0:
                    frequency = findFreq(filename, substr, &bytesCount);
                    if (frequency != ERR_FILE_OPEN && frequency != ERR_FILE_CLOSE) {
//                        if (frequency != 0)
                        {
                            printf("Process's PID: %5d Full path: %100s Bytes count: %6d Matched words count: %6d\n",
                                   getpid(), filename, bytesCount, frequency);
                        }
                    }
                    exit(0);
                default:
                    ++processesCount;
            }
        }
    }
    if (closedir(dir) != 0) {
        perror("Error during attempt to close directory.\n");
        return ERR_DIR_CLOSE;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("You should enter 3 parameters:\n"
                      "    1. Directory's name\n"
                      "    2. String to find\n"
                      "    3. Maximum number of processes\n");
        return ERR_ARGS_COUNT;
    }

    maxProcessesCount = strtol(argv[3], NULL, 10);
    if (maxProcessesCount <= 1 || errno == ERANGE) {
        printf("Maximum number of processes must be more than 1 and less than LONG_MAX\n");
        return ERR_ARGS_NUM;
    }

    char *dirname = (char*)malloc(PATH_MAX * sizeof(char));
    strcpy(dirname, argv[1]);

    int errorCode = dirTraversal(dirname, argv[2]);

    while (processesCount > 1) {
        if (wait(NULL) == -1)
            perror("Error during attempt to wait process termination\n");
        --processesCount;
    }

    exit(errorCode);
}
