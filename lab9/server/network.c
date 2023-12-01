#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include "network.h"
#include "grading_queue.h"
#include "compiler.h"
#define BUFFER_SIZE 1024

// decalrations

// int compileAndExecute(int newsockfd, int tid);
// int dequeueGradingRequest();
// void error(const char *msg);


int min(int a, int b) {
    return a < b ? a : b;
}

void action(int result, int newsockfd, int n, int tid)
{
    // Send response based on the result
    if (result == 0)
    {
        n = write(newsockfd, "PASS", 4);
    }
    else if (result == 1)
    {
        // Send compiler error details
        char compileErrorBuffer[BUFFER_SIZE];

        char compilerfile[50];
        sprintf(compilerfile, "client%d/compile_error.txt", tid);
        FILE *compileErrorFile = fopen(compilerfile, "r");
        if (compileErrorFile != NULL)
        {
            size_t bytesRead = fread(compileErrorBuffer, 1, sizeof(compileErrorBuffer) - 1, compileErrorFile);
            compileErrorBuffer[bytesRead] = '\0'; // Add null-terminator
            fclose(compileErrorFile);

            char message[BUFFER_SIZE] = "COMPILER ERROR\n";
            strcat(message, compileErrorBuffer);
            write(newsockfd, message, strlen(message));
        }
        else
        {
            n = write(newsockfd, "COMPILER ERROR (Details not available)\n", 39);
        }
    }
    else if (result == 2)
    {
        // Send runtime error details
        char runtimeErrorBuffer[BUFFER_SIZE];
        char runtimefile[50];
        sprintf(runtimefile, "client%d/runtime_error.txt", tid);
        FILE *runtimeErrorFile = fopen(runtimefile, "r");
        if (runtimeErrorFile != NULL)
        {
            size_t bytesRead = fread(runtimeErrorBuffer, 1, sizeof(runtimeErrorBuffer) - 1, runtimeErrorFile);
            runtimeErrorBuffer[bytesRead] = '\0'; // Add null-terminator
            fclose(runtimeErrorFile);

            char message[BUFFER_SIZE] = "RUNTIME ERROR\n";
            strcat(message, runtimeErrorBuffer);
            write(newsockfd, message, strlen(message));
        }
        else
        {
            n = write(newsockfd, "RUNTIME ERROR (Details not available)\n", 38);
        }
    }
    else if (result == 3)
    {
        // Send output error details
        char diffBuffer[BUFFER_SIZE];
        char difffile[50];
        sprintf(difffile, "client%d/diff.txt", tid);
        FILE *diffFile = fopen(difffile, "r");
        if (diffFile != NULL)
        {
            size_t bytesRead = fread(diffBuffer, 1, sizeof(diffBuffer) - 1, diffFile);
            diffBuffer[bytesRead] = '\0';
            fclose(diffFile);

            char message[BUFFER_SIZE] = "OUTPUT ERROR\n";
            strcat(message, diffBuffer);
            write(newsockfd, message, strlen(message));
        }
        else
        {
            n = write(newsockfd, "OUTPUT ERROR (Details not available)\n", 37);
        }
    }

    if (n < 0)
        error("ERROR writing to socket");
    close(newsockfd);
}

void *workerThread(void *arg)
{
    while (1)
    {
        int newsockfd = dequeueGradingRequest();

        // Process grading request
        char buffer[BUFFER_SIZE];
        int n;
        char command1[50];
        int tid = (pid_t) syscall(SYS_gettid);
        sprintf(command1, "mkdir -p client%d", tid);
        system(command1);

        bzero(buffer, BUFFER_SIZE);
        int length;
        n = read(newsockfd, &length, sizeof(int));
        if (n <= 0)
        {
            printf("2. error reading from socket");
        }
        char sourcefilepath[50];
        sprintf(sourcefilepath, "client%d/temp.c", tid);
        FILE *sourceFile = fopen(sourcefilepath, "w");
        if (sourceFile == NULL)
        {
            error("Error creating temporary source file"); // COMPILER ERROR
        }

        bzero(buffer, BUFFER_SIZE);
        n = read(newsockfd, buffer, min(BUFFER_SIZE - 1, length));
        if (n > 0)
        {
            fwrite(buffer, n, 1, sourceFile);
        }
        else if (n < 0)
        {
            error("1. ERROR reading from socket");
        }

        printf("-->Received source code\n");
        fclose(sourceFile);
        // Compile and execute the code
        int result = compileAndExecute(newsockfd, tid);

        action(result, newsockfd, n, tid);

        char command2[50];
        sprintf(command2, "rm -r client%d", tid);
        system(command2);
    }
    return NULL;
}
