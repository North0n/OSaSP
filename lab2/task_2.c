#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 32

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("You should enter filename as parameter\n");
        return 1;
    }

    const char *filename = argv[1];
    int fDescriptor;
    if ((fDescriptor = open(filename, O_RDONLY)) < 0) {
        printf("Error during file opening attempt. Error code: %d\n", fDescriptor);
        return fDescriptor;
    }

    char buffer[BUFFER_SIZE + 1];
    int errorCode;
    while ((errorCode = read(fDescriptor, buffer, BUFFER_SIZE)) > 0) {
        buffer[BUFFER_SIZE] = '\0';
        puts(buffer);
    }
    if (errorCode < 0) {
        printf("Error during file reading attempt. Error code: %d\n", errorCode);
    }

    if (errorCode = close(fDescriptor)) {
        printf("Error during file closing attempt. May cause loss of data. Error code: %d", errorCode);
        return errorCode;
    }

    return 0;
}