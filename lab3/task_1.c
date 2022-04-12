#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

void printTime()
{
    char timeStr[200];
    struct timespec time;
    struct tm *parsedTime;

    if (clock_gettime(CLOCK_REALTIME, &time) == -1) {
        perror("Error occurred during attempt to get time\n");
        exit(EXIT_FAILURE);
    }

    if ((parsedTime = localtime(&time.tv_sec)) == NULL) {
        perror("Error occurred during attempt to parse time using localtime\n");
        exit(EXIT_FAILURE);
    }

    if (strftime(timeStr, sizeof(timeStr), "%T", parsedTime) == 0) {
        fprintf(stderr, "Time str exceed given size. Strftime returned 0\n");
        exit(EXIT_FAILURE);
    }

    printf("Time: %s:%ld\n", timeStr, time.tv_nsec / 1000000);
}

// Returns amount of created processes
int handleProcess()
{
    switch (fork()) {
        case -1:
            perror("Error during attempt to start another process\n");
            return 0;
        case 0:
            printf("Process's PID: %6d Process parent's PID: %6d\n", getpid(), getppid());
            printTime();
            exit(0);
        default:
            return 1;
    }
}

int main(int argc, char *argv[])
{
    int processesCount = 0;
    processesCount += handleProcess();
    processesCount += handleProcess();
    printf("Process's PID: %6d Process parent's PID: %6d\n", getpid(), getppid());
    printTime();

    char *execName = NULL, *command = NULL;
    if ((execName = calloc(sizeof(char), FILENAME_MAX)) == NULL) {
        perror("Error during attempt to allocate memory\n");
        if (system("ps -x") == -1)
            perror("Error during attempt to execute \"ps -x\"");
        goto WaitForChildren;
    }
    strcpy(execName, strrchr(argv[0], '/') + 1);
    if ((command = calloc(sizeof(char), FILENAME_MAX + 6)) == NULL) {
        perror("Error during attempt to allocate memory\n");
        if (system("ps -x") == -1)
            perror("Error during attempt to execute \"ps -x\"");
        goto WaitForChildren;
    }
    sprintf(command, "ps -C %s", execName);
    if (system(command) == -1)
        perror("Error during attempt to execute \"ps\"");

WaitForChildren:
    free(command);
    free(execName);

    for (int i = 0; i < processesCount; ++i)
        if (wait(NULL) == -1)
            perror("Error during attempt to wait process termination\n");

    return 0;
}