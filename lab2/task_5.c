#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

int closeFiles(FILE *inpFile, FILE *outFile)
{
    int error = 0;
    if (fclose(inpFile) == EOF) {
        printf("Error during attempt to close first file. May cause loss of data.\n");
        error = 1;
    }
    if (fclose(outFile) == EOF) {
        printf("Error during attepmt to close second file. May cause loss of data.\n");
        error = 1;
    }
    return error;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("You should enter 2 parameters:\n");
        printf("    First - input file's name\n");
        printf("    Second - output file's name\n");
        return 1;
    }

    const char *inputFileName = argv[1];
    const char *outputFileName = argv[2];
    FILE *inputFile, *outputFile;
    if ((inputFile = fopen(inputFileName, "r")) == NULL) {
        printf("Error during first file opening attempt\n");
        return 1;
    }
    if ((outputFile = fopen(outputFileName, "w")) == NULL) {
        printf("Error during second file opening attempt\n");
        return 1;
    }

    struct stat accessRights;
    if (stat(inputFileName, &accessRights)) {
        printf("Error during attempt to access information about first file\n");
        closeFiles(inputFile, outputFile);
        return 1;
    }
    if (chmod(outputFileName, accessRights.st_mode)) {
        printf("Error during attempt to give access rights of first file to the second file\n");
        closeFiles(inputFile, outputFile);
        return 1;
    }

    char ch;
    while ((ch = getc(inputFile)) != EOF) {
        if (putc(ch, outputFile) == EOF) {
            printf("Error during attempt to write to the second file\n");
            break;
        }
    }

    return closeFiles(inputFile, outputFile);
}