#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#define ERR_ARGS_COUNT (-1)
#define ERR_FILE_OPEN (-2)
#define ERR_FILE_CLOSE (-3)
#define ERR_OPEN_DIR (-4)
#define ERR_CLOSE_DIR (-5)
#define ERR_SIG_ASSIGN (-6)
#define ERR_MAX_PROC (-7)

int processesCount = 1;

void processExited(int x)
{
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        --processesCount;
    }
}

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

int dirTraversal(char *dirname, const char *str, int maxProc)
{
    DIR *dir;
    if ((dir = opendir(dirname)) == NULL) {
        perror("Error during attempt to open directory.\n");
        return ERR_OPEN_DIR;
    }
    struct dirent *dirent;
    struct stat stats;
    int length = (int)strlen(dirname);
    int error = 0;
    int frequency = 0;
    int lettersCount = 0;
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
        if (S_ISDIR(stats.st_mode)) {
            error |= dirTraversal(dirname, str, maxProc);
        } else if (S_ISREG(stats.st_mode)) {
            while (processesCount >= maxProc) {
                processExited(0);
            }
            pid_t pid = fork();
            switch (pid) {
                case -1:
                    perror("Error during attempt to start another process\n");
                    break;
                case 0:
                    frequency = findFreq(dirname, str, &lettersCount);
                    if (frequency != ERR_FILE_CLOSE && frequency != ERR_FILE_OPEN) {
//                        if (frequency != 0)
                        {
                            printf("Process's PID: %5d Full path: %100s Bytes count: %6d Matched words count: %6d\n",
                                   getpid(), dirname, lettersCount, frequency);
                        }
                    }
                    _exit(0);
                default:
                    ++processesCount;
                    break;
            }
        }
    }
    if (closedir(dir)) {
        perror("Error during attempt to close directory.\n");
        return ERR_CLOSE_DIR;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf("You should enter 3 parameters:\n");
        printf("    1. Directory's name\n");
        printf("    2. String to find\n");
        printf("    3. Maximum number of processes\n");
        return ERR_ARGS_COUNT;
    }

    int maxProc = (int)strtol(argv[3], NULL, 10);
    if (maxProc <= 0) {
        printf("Maximum number of processes must be more than 0\n");
        return ERR_MAX_PROC;
    }

    if (signal(SIGCHLD, processExited) == SIG_ERR) {
        perror("Error during attempt to assign signal to a process termination\n");
        return ERR_SIG_ASSIGN;
    }

    char *dirname = (char*)malloc(PATH_MAX * sizeof(char));
    strcpy(dirname, argv[1]);

    int errorCode = dirTraversal(dirname, argv[2], maxProc);

    _exit(errorCode);
}
