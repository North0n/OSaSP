#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <memory.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define ERR_SIGNAL_SET 1
#define ERR_SIGNAL_ACTION 2
#define ERR_SIGNAL_SET_PROC_MASK 4
#define ERR_SIGNAL_SEND 8
#define ERR_PROC_CREATION 16
#define ERR_TIME_GET 32
#define ERR_SEM_CREAT 64
#define ERR_SEM_WAIT 128
#define ERR_SEM_POST 256

#define CHILDREN_COUNT 2
// Sleep time in microseconds
#define SLEEP_TIME 100 * 1000
#define SEMAPHORE_NAME "/sync_start"

int messagesCount = 0;
pid_t children[CHILDREN_COUNT];

long getTimeMilliseconds()
{
    struct timespec time;

    if (clock_gettime(CLOCK_REALTIME, &time) == -1) {
        perror("Error occurred during attempt to get time\n");
        exit(ERR_TIME_GET);
    }

    return time.tv_sec * 1000 + time.tv_nsec / 1000000;
}

// Handler for signal received from parent
void handlerSigUsr1(int sig, siginfo_t *siginfo, void *code)
{
    printf("%d Pid: %d Ppid: %d Time: %ld Child%d GET SIGUSR1\n",
           messagesCount, getpid(), getppid(), getTimeMilliseconds(), getpid() == children[0] ? 1 : 2);

    if (kill(getppid(), SIGUSR2)) {
        fprintf(stderr, "Child%d failed to send SIGUSR2", getpid() == children[0] ? 1 : 2);
        perror("");
    }
    printf("%d Pid: %d Ppid: %d Time: %ld Child%d SEND SIGUSR2\n",
           messagesCount, getpid(), getppid(), getTimeMilliseconds(), getpid() == children[0] ? 1 : 2);
    ++messagesCount;
}

// Handler for signal received from child
void handlerSigUsr2(int sig, siginfo_t *siginfo, void *code)
{
    printf("%d Pid: %d Ppid: %d Time: %ld Parent GET SIGUSR2 from Child%d\n",
           messagesCount, getpid(), getppid(), getTimeMilliseconds(), siginfo->si_pid == children[0] ? 1 : 2);
    usleep(SLEEP_TIME);
    if (kill(0, SIGUSR1)) {
        perror("Parent failed to send SIGUSR1");
    }
    printf("%d Pid: %d Ppid: %d Time: %ld Parent SEND SIGUSR1 to Children\n",
           messagesCount, getpid(), getppid(), getTimeMilliseconds());
    ++messagesCount;
}

int main()
{
    memset(children, 0, CHILDREN_COUNT * sizeof(pid_t));
    sem_t *semaphore = sem_open(SEMAPHORE_NAME, O_CREAT, 0777, 0);
    if (semaphore == SEM_FAILED) {
        perror("Failed to create semaphore: ");
        exit(ERR_SEM_CREAT);
    }

    // Creating mask for SIGUSR2
    sigset_t mask;
    if (sigemptyset(&mask)) {
        perror("Failed to create signal set for SIGUSR2");
        exit(ERR_SIGNAL_SET);
    }
    if (sigaddset(&mask, SIGUSR2) == -1) {
        perror("Failed to add signal to set for SIGUSR2");
        exit(ERR_SIGNAL_SET);
    }

    // Setting blocking mask for SIGUSR2, so it would be promoted to each child
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1) {
        perror("Failed to set blocking mask for SIGUSR2 in parent");
        exit(ERR_SIGNAL_SET_PROC_MASK);
    }

    // Creating signal handler for SIGUSR2
    struct sigaction action;
    action.sa_mask = mask;
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = handlerSigUsr2;
    if (sigaction(SIGUSR2, &action, NULL)) {
        perror("Failed to add action handler for SIGUSR2");
        exit(ERR_SIGNAL_ACTION);
    }

    // Creating mask for SIGUSR1
    if (sigemptyset(&mask)) {
        perror("Failed to create signal set for SIGUSR2");
        exit(ERR_SIGNAL_SET);
    }
    if (sigaddset(&mask, SIGUSR1) == -1) {
        perror("Failed to add signal to set for SIGUSR1");
        exit(ERR_SIGNAL_SET);
    }

    // Creating signal handler for SIGUSR1
    action.sa_mask = mask;
    action.sa_flags = SA_SIGINFO;
    action.sa_sigaction = handlerSigUsr1;
    if (sigaction(SIGUSR1, &action, NULL)) {
        perror("Failed to add action handler for SIGUSR1");
        exit(ERR_SIGNAL_ACTION);
    }

    // Prints info about parent's process
    printf("Pid: %d Ppid: %d Time: %ld Parent\n", getpid(), getppid(), getTimeMilliseconds());
    for (int i = 0; i < CHILDREN_COUNT; ++i) {
        children[i] = fork();
        switch (children[i]) {
            // Child process
            case 0:
                children[i] = getpid();
                printf("Pid: %d Ppid: %d Time: %ld Child%d\n", getpid(), getppid(), getTimeMilliseconds(), i + 1);
                if (sem_post(semaphore)) {
                    perror("Failed to post on semaphore: ");
                    exit(ERR_SEM_POST);
                }
                while (42);
            // Error
            case -1:
                perror("Failed to create child process");
                exit(ERR_PROC_CREATION);
        }
    }

    // Waiting for both of processes to increment semaphore
    if (sem_wait(semaphore) || sem_wait(semaphore)) {
        perror("Failed to wait on semaphore: ");
        exit(ERR_SEM_WAIT);
    }

    if (sem_close(semaphore) || sem_unlink(SEMAPHORE_NAME)) {
        perror("Failed to close and unlink semaphore: ");
    }

    // Replacing mask with mask blocking SIGUSR1 instead of SIGUSR2
    if (sigprocmask(SIG_SETMASK, &mask, NULL)) {
        perror("Failed to set blocking mask for SIGUSR1 in parent");
        exit(ERR_SIGNAL_SET_PROC_MASK);
    }

    if (kill(0, SIGUSR1) == -1) {
        perror("Parent failed to send SIGUSR1");
        exit(ERR_SIGNAL_SEND);
    }
    while (42);
}