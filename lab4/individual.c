#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>

#define ERR_SHM_OPEN                1
#define ERR_SHM_CLOSE               2
#define ERR_SHM_UNLINK              3
#define ERR_TRUNCATE                4
#define ERR_MMAP                    5
#define ERR_MUNMAP                  6
#define ERR_FORK                    7
#define ERR_SEM_OPEN                8
#define ERR_SEM_WAIT                9
#define ERR_SEM_POST                10
#define ERR_SEM_CLOSE               11
#define ERR_SEM_UNLINK              12
#define ERR_SET_PGID                13
#define ERR_SET_EMPTY               14
#define ERR_SET_ADD                 15
#define ERR_SIG_ACTION              16
#define ERR_SIG_SEND                17

typedef struct
{
    pid_t pid;
    pid_t ppid;
    pid_t pgid;
    int childrenCount;
    struct sigaction action;

} MyProcess;

#define PROCESSES_COUNT 9
#define SHARED_MEMORY_NAME "/processes tree"
#define SHARED_MEMORY_SIZE (PROCESSES_COUNT * sizeof(MyProcess))
#define SEM_CREATION_NAME "/tree creation"
#define SEM_INIT_NAME "/tree initialization"

const char ReceivingSignals[PROCESSES_COUNT] = {0, SIGUSR2, SIGUSR1, SIGUSR1, SIGUSR2, SIGUSR1, SIGUSR1, SIGUSR1, SIGUSR1};

const char SendingSignals[PROCESSES_COUNT] = {0, SIGUSR1, 0, SIGUSR2, SIGUSR1, 0, 0, SIGUSR1, SIGUSR2};

void handle_error(const char *errorMsg, int exit_code)
{
    perror(errorMsg);
    shm_unlink(SHARED_MEMORY_NAME);
    sem_unlink(SEM_CREATION_NAME);
    sem_unlink(SEM_INIT_NAME);
    exit(exit_code);
}

int procIndex = 0;
MyProcess *sharedMemPtr;
int sigCount = 0;

void receiveUsr(int signal, siginfo_t* signalInfo, void* context)
{
    ++sigCount;

    struct timespec time;
    long timeMcs = 0;
    if (clock_gettime(CLOCK_REALTIME, &time) == -1)
        perror("Failed to get time");
    else
        timeMcs = time.tv_sec + time.tv_nsec / 1000;

    printf("%d Pid: %d Ppid: %d received SIGUSR%d Time: %ld\n", sigCount, getpid(), getppid(), ReceivingSignals[procIndex] == SIGUSR1 ? 1 : 2, timeMcs);

    if (SendingSignals[procIndex] == 0)
        return;
    if (procIndex == 1 && sigCount == 101) {
        if (kill(getpid(), SIGTERM) == -1)
            handle_error("Failed to send SIGTERM", ERR_SIG_SEND);
        return;
    }
    if (kill(-sharedMemPtr[procIndex % (PROCESSES_COUNT - 1) + 1].pgid, SendingSignals[procIndex]) == -1)
        perror("Failed to send signal");
    else
        printf("%d Pid: %d Ppid: %d sent SIGUSR%d Time: %ld\n", sigCount, getpid(), getppid(), SendingSignals[procIndex] == SIGUSR1 ? 1 : 2, timeMcs);
}

void receiveTerm(int signal, siginfo_t* signalInfo, void* context)
{
    if (sharedMemPtr[procIndex].childrenCount != 0) {
        if (kill(-sharedMemPtr[procIndex + 1].pgid, SIGTERM))
            handle_error("Failed to send SIGTERM to children", ERR_SIG_SEND);
        for (int i = 1; i <= sharedMemPtr[procIndex].childrenCount; ++i) {
            if (waitpid(sharedMemPtr[procIndex + i].pid, NULL, 0) == -1)
                perror("Failed to wait for child");
        }
    }
    printf("Pid: %d Ppid: %d finished the work after %d SIGUSR%d\n", getpid(), getppid(), sigCount, SendingSignals[procIndex] == SIGUSR1 ? 1 : 2);
    exit(0);
}

void createProcesses(MyProcess *mappedMemoryPtr, int processIndex)
{
    sem_t *semCreation = sem_open(SEM_CREATION_NAME, O_RDWR);
    if (semCreation == SEM_FAILED)
        handle_error("Failed to open semaphore "SEM_CREATION_NAME, ERR_SEM_OPEN);

    sem_t *semInit = sem_open(SEM_INIT_NAME, O_RDWR);
    if (semInit == SEM_FAILED)
        handle_error("Failed to open semaphore "SEM_INIT_NAME, ERR_SEM_OPEN);

    int thisSignalIndex = processIndex;
    for (int i = 0, isParent = 1; i < mappedMemoryPtr[thisSignalIndex].childrenCount && isParent; ++i) {
        ++processIndex;
        isParent = 0;
        if (sem_wait(semCreation) == -1)
            handle_error("Failed to wait on semCreation", ERR_SEM_WAIT);
        switch (fork()) {
            // Error
            case -1:
                handle_error("Failed to create new process", ERR_FORK);

            // Created process
            case 0:
                procIndex = processIndex;
                sharedMemPtr = mappedMemoryPtr;

                mappedMemoryPtr[processIndex].pid = getpid();
                mappedMemoryPtr[processIndex].ppid = getppid();

                // Set signal handler
                if (sigaction(ReceivingSignals[processIndex], &mappedMemoryPtr[processIndex].action, NULL) == -1)
                    handle_error("Failed to set sigaction", ERR_SIG_ACTION);

                // Set SIGTERM handler
                sigset_t mask;
                if (sigemptyset(&mask))
                    handle_error("Failed to create empty mask", ERR_SET_EMPTY);
                if (sigaddset(&mask, SIGTERM))
                    handle_error("Failed to add SIGTERM to mask", ERR_SET_ADD);

                // Set PGID
                int err;
                if (mappedMemoryPtr[processIndex - 1].childrenCount == 0) {
                    err = setpgid(0, mappedMemoryPtr[processIndex - 1].pgid);
                } else {
                    err = setpgid(0, 0);
                }
                if (err == -1) {
                    handle_error("Failed to set process's group id", ERR_SET_PGID);
                }
                mappedMemoryPtr[processIndex].pgid = getpgid(0);

                struct sigaction actionTerm;
                actionTerm.sa_flags = SA_SIGINFO;
                actionTerm.sa_mask = mask;
                actionTerm.sa_sigaction = receiveTerm;

                if (sigaction(SIGTERM, &actionTerm, NULL))
                    handle_error("Failed to set action for SIGTERM", ERR_SIG_ACTION);

                if (sem_post(semCreation))
                    handle_error("Failed to post in semaphore "SEM_CREATION_NAME, ERR_SEM_POST);
                if (sem_close(semCreation) == -1)
                    handle_error("Failed to close semaphore "SEM_CREATION_NAME, ERR_SEM_CLOSE);

                createProcesses(mappedMemoryPtr, processIndex);

                if (sem_post(semInit) == -1)
                    handle_error("Failed to post in semaphore "SEM_INIT_NAME, ERR_SEM_POST);
                if (sem_close(semInit) == -1)
                    handle_error("Failed to close semaphore "SEM_INIT_NAME, ERR_SEM_CLOSE);

                while (42);

            // Process creator
            default:
                isParent = 1;
        }
    }

    if (sem_close(semCreation) == -1)
        handle_error("Failed to close semaphore "SEM_CREATION_NAME, ERR_SEM_CLOSE);
    if (sem_close(semInit) == -1)
        handle_error("Failed to close semaphore "SEM_INIT_NAME, ERR_SEM_CLOSE);
}

void createTree(MyProcess *mappedMemoryPtr)
{
    sem_t *semCreation = sem_open(SEM_CREATION_NAME, O_CREAT, 777, 1);
    if (semCreation == SEM_FAILED)
        handle_error("Failed to create semaphore "SEM_CREATION_NAME, ERR_SEM_OPEN);

    sem_t *semInit = sem_open(SEM_INIT_NAME, O_CREAT, 777, 1);
    if (semInit == SEM_FAILED)
        handle_error("Failed to create semaphore "SEM_INIT_NAME, ERR_SEM_OPEN);

    // Setting children count for each process
    for (int i = 0; i < PROCESSES_COUNT; ++i)
        mappedMemoryPtr[i].childrenCount = 0;
    mappedMemoryPtr[0].childrenCount = 1;
    mappedMemoryPtr[1].childrenCount = 2;
    mappedMemoryPtr[3].childrenCount = 1;
    mappedMemoryPtr[4].childrenCount = 3;
    mappedMemoryPtr[7].childrenCount = 1;

    // Setting info about main process
    mappedMemoryPtr[0].pid = getpid();
    mappedMemoryPtr[0].pgid = getpgrp();
    mappedMemoryPtr[0].ppid = getppid();

    // Setting actions
    sigset_t mask;
    // Mask for blocking SIGUSR1 and SIGUSR2 at the same time
    // Processes can't receive both signals, so we don't care what signal to block in a certain process
    if (sigemptyset(&mask))
        handle_error("Failed to create empty mask", ERR_SET_EMPTY);
    if (sigaddset(&mask, SIGUSR1))
        handle_error("Failed to add signal to mask", ERR_SET_ADD);
    if (sigaddset(&mask, SIGUSR2))
        handle_error("Failed to add signal to mask", ERR_SET_ADD);

    for (int i = 0; i < PROCESSES_COUNT; ++i) {
        mappedMemoryPtr[i].action.sa_flags = SA_SIGINFO;
        mappedMemoryPtr[i].action.sa_mask = mask;
        mappedMemoryPtr[i].action.sa_sigaction = receiveUsr;
    }

    createProcesses(mappedMemoryPtr, 0);

    // Waiting for all processes to be configured
    for (int i = 0; i < PROCESSES_COUNT; ++i) {
        if (sem_wait(semInit) == -1)
            handle_error("Failed to wait on semCreation "SEM_CREATION_NAME, ERR_SEM_WAIT);
    }

    if (sem_close(semInit) == -1)
        handle_error("Failed to close semaphore"SEM_INIT_NAME, ERR_SEM_CLOSE);

    if (sem_unlink(SEM_INIT_NAME) == -1)
        handle_error("Failed to unlink semaphore"SEM_INIT_NAME, ERR_SEM_UNLINK);

    if (sem_close(semCreation) == -1)
        handle_error("Failed to close semaphore "SEM_CREATION_NAME, ERR_SEM_CLOSE);

    if (sem_unlink(SEM_CREATION_NAME) == -1)
        handle_error("Failed to unlink semaphore "SEM_CREATION_NAME, ERR_SEM_UNLINK);
}

void mainHandlerInt(int signal, siginfo_t* signalInfo, void* context)
{
    if (kill(sharedMemPtr[1].pid, SIGTERM) == -1)
        handle_error("Failed to send SIGTERM to the first child of main process", ERR_SIG_SEND);
    printf("SIGTERM SENT\n");
    exit(0);
}

int main()
{
    int shmDescriptor = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 777);
    if (shmDescriptor == -1)
        handle_error("Failed to create shared memory object "SHARED_MEMORY_NAME, ERR_SHM_OPEN);
    if (ftruncate(shmDescriptor, SHARED_MEMORY_SIZE) == -1)
        handle_error("Failed to truncate shared memory object "SHARED_MEMORY_NAME, ERR_TRUNCATE);
    sharedMemPtr = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmDescriptor, 0);
    if (sharedMemPtr == MAP_FAILED)
        handle_error("Failed to mmap area", ERR_MMAP);

    createTree(sharedMemPtr);

    // Setting handler for SIGINT, so if user presses Ctrl+C all processes would be terminated
    sigset_t mask;
    if (sigemptyset(&mask) == -1)
        handle_error("Failed to create empty mask", ERR_SET_EMPTY);
    if (sigaddset(&mask, SIGINT) == -1)
        handle_error("Failed to add SIGTERM to mask", ERR_SET_ADD);

    struct sigaction actTerm;
    actTerm.sa_flags = SA_SIGINFO;
    actTerm.sa_mask = mask;
    actTerm.sa_sigaction = mainHandlerInt;

    if (sigaction(SIGINT, &actTerm, NULL) == -1)
        handle_error("Failed to set handler for SIGINT in main process", ERR_SIG_ACTION);

    // Displays process tree
//    for (int i = 0; i < PROCESSES_COUNT; ++i)
//        printf("%d PID %d PPID %d Children's count %d Group id %d\n", i, sharedMemPtr[i].pid, sharedMemPtr[i].ppid, sharedMemPtr[i].childrenCount, sharedMemPtr[i].pgid);

    if (kill(sharedMemPtr[1].pid, SIGUSR2))
        handle_error("Failed to send first signal to first child", ERR_SIG_SEND);

    if (waitpid(sharedMemPtr[1].pid, NULL, 0) == -1)
        perror("Failed to wait for the first child");

    if (close(shmDescriptor) == -1)
        handle_error("Failed to close shared memory object descriptor "SHARED_MEMORY_NAME, ERR_SHM_CLOSE);
    if (munmap(sharedMemPtr, SHARED_MEMORY_SIZE) == -1)
        handle_error("Failed to unmap area", ERR_MUNMAP);
    if (shm_unlink(SHARED_MEMORY_NAME))
        handle_error("Failed to unlink shared memory object "SHARED_MEMORY_NAME, ERR_SHM_UNLINK);
    return 0;
}
