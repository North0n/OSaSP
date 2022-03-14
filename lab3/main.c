#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#define SECS_IN_DAY (24 * 60 * 60)
#define TIMEZONE 3

#define ERROR_GETTING_TIME 0x01

static void displayTime()
{
    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("Error occurred during attempt to get time\n");
        _exit(ERROR_GETTING_TIME);
    }

    printf("%02ld:%02ld:%02ld:%03ld\n",
           (ts.tv_sec % SECS_IN_DAY) / 3600 + TIMEZONE,
           (ts.tv_sec % 3600) / 60,
           ts.tv_sec % 60,
           ts.tv_nsec / 1000000);
}

int main()
{
    pid_t pid1;
    pid_t pid2;
    pid1 = fork();
    if (pid1 != 0) {
        pid2 = fork();
    }
    printf("Process's PID: %6d Process parent's PID: %6d\n", getpid(), getppid());
    printf("Time: ");
    displayTime();
    if (pid1 != 0 && pid2 != 0) {
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    }
    return 0;
}