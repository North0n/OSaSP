#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "You should enter 1 parameter:\n");
        fprintf(stderr, "    First - filename\n");
        return -1;
    }

    int fDescriptor;
    FILE *file;
    if ((file = fopen(argv[1], "r")) == NULL) {
        perror("Error during attempt to open file\n");
        return -2;
    }

    char ch;
    while ((ch = getc(file)) != EOF) {
        if (putchar(ch) == EOF) {
            perror("Error during attempt to write to the output\n");
            break;
        }
    }

    if (fclose(file) == EOF) {
        perror("Error during file closing attempt. May cause loss of data\n");
        return -3;
    }

    return 0;
}