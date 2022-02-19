#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("You should enter 1 parameter:\n");
        printf("    First - filename\n");
        return 1;
    }

    int fDescriptor;
    FILE *file;
    if ((file = fopen(argv[1], "r")) == NULL) {
        printf("Error during attempt to open file\n");
        return 1;
    }

    char ch;
    while ((ch = getc(file)) != EOF) {
        if (putchar(ch) == EOF) {
            printf("Error during attempt to write to the output\n");
            break;
        }
    }

    if (fclose(file) == EOF) {
        printf("Error during file closing attempt. May cause loss of data\n");
        return 1;
    }

    return 0;
}