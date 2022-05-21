#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define ERR_TIME_GET                    1
#define ERR_TIME_PARSE                  2
#define ERR_TIME_TO_STR                 3
#define ERR_THREAD_CREATE               4
#define ERR_THREAD_JOIN                 5

#define handle_error(str, exit_code) { \
    perror(str);                       \
    exit(exit_code);                   \
}

#define THREADS_COUNT 2

void* displayInfo(void *data)
{
    char timeStr[200];
    struct timespec time;
    struct tm *parsedTime;

    if (clock_gettime(CLOCK_REALTIME, &time) == -1) {
        perror("Error occurred during attempt to get time\n");
        exit(ERR_TIME_GET);
    }

    if ((parsedTime = localtime(&time.tv_sec)) == NULL) {
        perror("Error occurred during attempt to parse time using localtime\n");
        exit(ERR_TIME_PARSE);
    }

    if (strftime(timeStr, sizeof(timeStr), "%T", parsedTime) == 0) {
        fprintf(stderr, "Time str exceed given size. Strftime returned 0\n");
        exit(ERR_TIME_TO_STR);
    }

    printf("Id: %lu Pid: %d Time: %s:%ld\n", pthread_self(), getpid(), timeStr, time.tv_nsec / 1000000);

    return 0;
}

int main() {

    displayInfo(NULL);

    pthread_t threads[THREADS_COUNT];
    for (int i = 0; i < THREADS_COUNT; ++i) {
        if (pthread_create(&threads[i], NULL, displayInfo, NULL))
            handle_error("Failed to create thread1", ERR_THREAD_CREATE);
    }

    for (int i = 0; i < THREADS_COUNT; ++i) {
        if (pthread_join(threads[i], NULL))
            handle_error("Failed to join process", ERR_THREAD_JOIN);
    }

    return 0;
}
