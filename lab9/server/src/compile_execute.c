#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "compile_execute.h"
#define BUFFER_SIZE 1024

// Function to compile and execute the submitted code
int compileAndExecute(int newsockfd, int tid)
{

    // Compile the code
    char command1[100];
    sprintf(command1, "gcc -o client%d/temp client%d/temp.c 2> client%d/compile_error.txt", tid, tid, tid);
    int compileStatus = system(command1);

    if (compileStatus != 0)
    {
        return 1; // COMPILER ERROR
    }

    // Execute the compiled code
    char command2[100];
    sprintf(command2, "./client%d/temp > client%d/program_output.txt 2> client%d/runtime_error.txt", tid, tid, tid);
    int executionStatus = system(command2);

    if (executionStatus != 0)
    {
        return 2; // RUNTIME ERROR
    }

    // Read the program's output
    char outputfilepath[50];
    sprintf(outputfilepath, "client%d/program_output.txt", tid);
    FILE *outputFile = fopen(outputfilepath, "r");
    if (outputFile == NULL)
    {
        perror("Error opening program output file");
        return 3; // OUTPUT ERROR
    }

    char outputBuffer[BUFFER_SIZE];
    char desiredOutput[] = "1 2 3 4 5 6 7 8 9 10";

    char desiredfilepath[50];
    sprintf(desiredfilepath, "client%d/desired_output.txt", tid);
    FILE *desiredFile = fopen(desiredfilepath, "w");
    if (desiredFile == NULL)
    {
        perror("Error creating temporary desired output file");
        return -1;
    }
    fprintf(desiredFile, "%s", desiredOutput);
    fclose(desiredFile);

    if (fgets(outputBuffer, sizeof(outputBuffer), outputFile) == NULL)
    {
        fclose(outputFile);
        return 3; // OUTPUT ERROR
    }

    fclose(outputFile);

    // Compare the program's output with desired output
    if (strcmp(outputBuffer, desiredOutput) != 0)
    {
        // Create a diff file to show the difference
        char command3[100];
        sprintf(command3, "diff -u client%d/program_output.txt client%d/desired_output.txt > client%d/diff.txt", tid, tid, tid);
        int diffStatus = system(command3);
        return 3; // OUTPUT ERROR
    }

    return 0; // PASS
}
