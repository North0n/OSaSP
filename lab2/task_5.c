#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

int closeFiles(FILE *inpFile, FILE *outFile)
{
    int error = 0;
    if (fclose(inpFile) == EOF) {
        perror("Error during attempt to close first file. May cause loss of data.\n");
        error = -3;
    }
    if (fclose(outFile) == EOF) {
        perror("Error during attepmt to close second file. May cause loss of data.\n");
        error = -3;
    }
    return error;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "You should enter 2 parameters:\n");
        fprintf(stderr, "    First - input file's name\n");
        fprintf(stderr, "    Second - output file's name\n");
        return -1;
    }

    const char *inputFileName = argv[1];
    const char *outputFileName = argv[2];
    FILE *inputFile, *outputFile;
    if ((inputFile = fopen(inputFileName, "r")) == NULL) {
        perror("Error during first file opening attempt\n");
        return -2;
    }
    if ((outputFile = fopen(outputFileName, "w")) == NULL) {
        perror("Error during second file opening attempt\n");
        return -2;
    }

    struct stat accessRights;
    if (stat(inputFileName, &accessRights)) {
        perror("Error during attempt to access information about first file\n");
        closeFiles(inputFile, outputFile);
        return -4;
    }
    if (chmod(outputFileName, accessRights.st_mode)) {
        perror("Error during attempt to give access rights of first file to the second file\n");
        closeFiles(inputFile, outputFile);
        return -4;
    }

    char ch;
    while ((ch = getc(inputFile)) != EOF) {
        if (putc(ch, outputFile) == EOF) {
            perror("Error during attempt to write to the second file\n");   
            break;
        }
    }

    return closeFiles(inputFile, outputFile);
}