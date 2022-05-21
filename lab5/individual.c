#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <fcntl.h>
#include <pthread.h>

#define ERR_ARGS_COUNT              1
#define ERR_ARGS_NUM                2
#define ERR_ARGS_SUBSTR_LENGTH      12
#define ERR_SEM_CREATE              5
#define ERR_SEM_CLOSE               6
#define ERR_SEM_UNLINK              7
#define ERR_SEM_WAIT                8
#define ERR_SEM_POST                9
#define ERR_FILE_OPEN               10
#define ERR_FILE_CLOSE              11
#define ERR_DIR_OPEN                32
#define ERR_DIR_CLOSE               64

#define SEM_THREAD_CONTROL_NAME     "/sem thread count control"
#define MAX_SUBSTR_LENGTH           255

#define handle_error(str, exit_code) {    \
    perror(str);                          \
    exit(exit_code);                      \
    sem_unlink(SEM_THREAD_CONTROL_NAME);  \
}

typedef struct
{
    char *filename;
    const char *substr;
} ToFind;

long maxThreadsCount = 0;
sem_t *semThreadCountControl;

void *findSubstr(void *toFindStruct)
{
    char *filename = ((ToFind *)toFindStruct)->filename;
    const char *substr = ((ToFind *)toFindStruct)->substr;
    free(toFindStruct);

    FILE *file;
    if ((file = fopen(filename, "r")) == NULL) {
        fprintf(stderr, "Filename: %s ", filename);
        perror("Error occurred during attempt to open file");
        if (sem_post(semThreadCountControl) == -1) {
            handle_error("Failed to post on semaphore", ERR_SEM_POST);
        }
        free(filename);
        return (void*)ERR_FILE_OPEN;
    }

    int ch;
    unsigned long index = 0;
    int frequency = 0;
    unsigned long substrLen = strlen(substr);
    int lettersCount = 0;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == substr[index]) {
            index = (index + 1) % substrLen;
            if (index == 0)
                ++frequency;
        } else {
            index = 0;
        }
        ++lettersCount;
    }

    if (fclose(file) == EOF) {
        perror("Error occurred during attempt to close file, may cause loss of data");
        if (sem_post(semThreadCountControl) == -1) {
            handle_error("Failed to post on semaphore", ERR_SEM_POST);
        }
        free(filename);
        return (void*)ERR_FILE_CLOSE;
    }

    if (frequency != 0)
        printf("Id: %lul Path: %s Bytes count: %d Frequency: %d\n", pthread_self(), filename, lettersCount, frequency);
    free(filename);

    if (sem_post(semThreadCountControl) == -1) {
        handle_error("Failed to post on semaphore", ERR_SEM_POST);
    }
    return NULL;
}

int dirTraversal(char *dirname, const char *substr)
{
    DIR *dir;
    if ((dir = opendir(dirname)) == NULL) {
        perror("Error during attempt to open directory ");
        return ERR_DIR_OPEN;
    }

    struct dirent* dirEntry;
    int error = 0;
    struct stat stats;
    int length = (int)strlen(dirname);

    while ((dirEntry = readdir(dir)) != NULL) {
        // Skip current and previous directory
        if (strcmp(dirEntry->d_name, ".") == 0 || strcmp(dirEntry->d_name, "..") == 0)
            continue;

        // Form current file dirname
        dirname[length] = '/';
        dirname[length + 1] = '\0';
        strcat(dirname, dirEntry->d_name);
        if (lstat(dirname, &stats) != 0) {
            perror("Failed to get info about file. ");
            fprintf(stderr, "Filename: %s\n", dirname);
            continue;
        }

        // Current file processing
        if (S_ISDIR(stats.st_mode))
            error |= dirTraversal(dirname, substr);
        else if (S_ISREG(stats.st_mode)) {
            // Ensures that thread count is less than or equal to maximum thread count
            if (sem_wait(semThreadCountControl) == -1) {
                perror("Failed to wait on semaphore"SEM_THREAD_CONTROL_NAME);
                continue;
            }
            // Creation of a new thread for file handling
            ToFind *argsPtr = (ToFind *)calloc(1, sizeof(ToFind));
            argsPtr->filename = (char*)calloc(strlen(dirname) + 1, sizeof(char));
            strcpy(argsPtr->filename, dirname);
            argsPtr->substr = substr;

            pthread_t thread;
            if (pthread_create(&thread, NULL, findSubstr, (void *)argsPtr)) {
                perror("Failed to start a new thread");
                continue;
            }
            if (pthread_detach(thread)) {
                perror("Failed to make thread detached");
                continue;
            }
        }
    }
    if (closedir(dir) != 0) {
        perror("Error during attempt to close directory ");
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
        exit(ERR_ARGS_COUNT);
    }

    maxThreadsCount = strtol(argv[3], NULL, 10);
    if (maxThreadsCount < 1 || errno == ERANGE) {
        printf("Maximum number of processes must be more than 0 and less than LONG_MAX\n");
        exit(ERR_ARGS_NUM);
    }
    if (strlen(argv[2]) >= MAX_SUBSTR_LENGTH) {
        printf("Maximum length of substring is 254 symbols\n");
        exit(ERR_ARGS_SUBSTR_LENGTH);
    }

    sem_unlink(SEM_THREAD_CONTROL_NAME);

    semThreadCountControl = sem_open(SEM_THREAD_CONTROL_NAME, O_CREAT, 777, maxThreadsCount);
    if (semThreadCountControl == SEM_FAILED)
        handle_error("Failed to create semThreadCountControl "SEM_THREAD_CONTROL_NAME, ERR_SEM_CREATE);

    char *dirname = (char*)calloc(PATH_MAX, sizeof(char));
    strcpy(dirname, argv[1]);
    int error = dirTraversal(dirname, argv[2]);

    // Waiting for all threads to finish
    for (int i = 0; i < maxThreadsCount; ++i) {
        if (sem_wait(semThreadCountControl) == -1) {
            perror("Failed to wait on semaphore");
        }
    }

    if (sem_close(semThreadCountControl) == -1)
        handle_error("Failed to close semThreadCountControl "SEM_THREAD_CONTROL_NAME, ERR_SEM_CLOSE);
    if (sem_unlink(SEM_THREAD_CONTROL_NAME) == -1)
        handle_error("Failed to unlink semThreadCountControl "SEM_THREAD_CONTROL_NAME, ERR_SEM_UNLINK);

    return error;
}
