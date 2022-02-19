#include <stdio.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("You should enter 1 parameter:\n");
        printf("    First - filename\n");
        return 1;
    }

    printf("Type ^F to exit.\n");
    const char *filename = argv[1];
    FILE *file;
    if ((file = fopen(filename, "w")) == NULL) {
        printf("Error during file opening attempt\n");
        return 1;
    }

    char ch;
    while ((ch = getc(stdin)) != 6) { // '^F' == 6
        if (fputc(ch, file) == EOF) {
            printf("Error during attempt to write to the file\n");
            break;
        }
    }

    if (fclose(file) == EOF) {
        printf("Error during file closing attempt. May cause loss of data.\n");
        return 1;
    }

    return 0;
}