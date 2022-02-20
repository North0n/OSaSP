#include <stdio.h>
#include <stdlib.h>

int printStr(FILE *inputFile, FILE *outputFile)
{
    int ch = getc(inputFile);
    while (ch != EOF && ch != '\n') {
        if (putc(ch, outputFile) == EOF)
            return EOF;
        ch = getc(inputFile);
    }
    if (ch == '\n' && putc(ch, outputFile) == EOF)
        return EOF;
    return ch;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("You should enter 2 parameters:\n");
        printf("    First - filename\n");
        printf("    Second - group size\n");
        return 1;
    }

    int groupSize = atoi(argv[2]);
    if (groupSize < 0) {
        printf("You entered wrong group size. It should be >=0\n");
        return 1;
    }
    FILE *file;
    if ((file = fopen(argv[1], "r")) == NULL) {
        printf("Error during file opening attempt\n");
        return 1;
    }

    int eof = 42;
    int counter;
    while (eof != EOF) {
        counter = 0;
        do {
            eof = printStr(file, stdout);
        } while (eof != EOF && ++counter != groupSize);
        if (eof != EOF) {
            printf("Press any key\n");
            while (getchar() != '\n');
        }
    }
    
    if (fclose(file) == EOF) {
        printf("Error during file closing attempt. May cause loss of data.\n");
        return 1;
    }

    return 0;
}