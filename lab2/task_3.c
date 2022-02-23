#include <stdio.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "You should enter 1 parameter:\n");
        fprintf(stderr, "    First - filename\n");
        return -1;
    }

    printf("Type ^F to exit.\n");
    FILE *file;
    if ((file = fopen(argv[1], "w")) == NULL) {
        perror("Error during file opening attempt\n");
        return -2;
    }

    char ch;
    while ((ch = getc(stdin)) != 6) { // '^F' == 6
        if (fputc(ch, file) == EOF) {
            perror("Error during attempt to write to the file\n");
            break;
        }
    }

    if (fclose(file) == EOF) {
        perror("Error during file closing attempt. May cause loss of data.\n");
        return -3;
    }

    return 0;
}