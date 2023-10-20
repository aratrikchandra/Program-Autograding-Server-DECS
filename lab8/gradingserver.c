#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <pthread.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}
int min(int a, int b)
{
    return a < b ? a : b;
}
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

    char outputBuffer[256];
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
        char compileErrorBuffer[256];

        char compilerfile[50];
        sprintf(compilerfile, "client%d/compile_error.txt", tid);
        FILE *compileErrorFile = fopen(compilerfile, "r");
        if (compileErrorFile != NULL)
        {
            size_t bytesRead = fread(compileErrorBuffer, 1, sizeof(compileErrorBuffer) - 1, compileErrorFile);
            compileErrorBuffer[bytesRead] = '\0'; // Add null-terminator
            fclose(compileErrorFile);
            // n = write(newsockfd, "COMPILER ERROR\n", 16);
            // usleep(100000); // 0.1 second delay
            // n = write(newsockfd, compileErrorBuffer, strlen(compileErrorBuffer));

            char message[1024] = "COMPILER ERROR\n";
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
        char runtimeErrorBuffer[256];
        char runtimefile[50];
        sprintf(runtimefile, "client%d/runtime_error.txt", tid);
        FILE *runtimeErrorFile = fopen(runtimefile, "r");
        if (runtimeErrorFile != NULL)
        {
            size_t bytesRead = fread(runtimeErrorBuffer, 1, sizeof(runtimeErrorBuffer) - 1, runtimeErrorFile);
            runtimeErrorBuffer[bytesRead] = '\0'; // Add null-terminator
            fclose(runtimeErrorFile);
            // n = write(newsockfd, "RUNTIME ERROR\n", 15);
            // usleep(100000); // 0.1 second delay
            // n = write(newsockfd, runtimeErrorBuffer, strlen(runtimeErrorBuffer));

            char message[1024] = "RUNTIME ERROR\n";
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
        char diffBuffer[256];
        char difffile[50];
        sprintf(difffile, "client%d/diff.txt", tid);
        FILE *diffFile = fopen(difffile, "r");
        if (diffFile != NULL)
        {
            size_t bytesRead = fread(diffBuffer, 1, sizeof(diffBuffer) - 1, diffFile);
            diffBuffer[bytesRead] = '\0';
            fclose(diffFile);
            // n = write(newsockfd, "OUTPUT ERROR\n", 14);
            // usleep(100000); // 0.1 second delay
            // n = write(newsockfd, diffBuffer, strlen(diffBuffer));

            char message[1024] = "OUTPUT ERROR\n";
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
void *handleclient(void *arg)
{
    int newsockfd = *((int *)arg);
    char buffer[256];
    int n;
    char command1[50];
    int tid=pthread_self();
    sprintf(command1, "mkdir -p client%d", tid);
    system(command1);

        bzero(buffer, 256);
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

        bzero(buffer, 256);
        n = read(newsockfd, buffer, min(255, length));
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

        // usleep(100000); // 0.1 second delay
    
    char command2[50];
    sprintf(command2, "rm -r client%d", tid);
    system(command2);

    pthread_exit(NULL);
}
int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    portno = atoi(argv[1]);
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 1000);

    clilen = sizeof(cli_addr);

    // int fds[1000];
    int *fds = (int *)malloc(sizeof(int) * 1000);

    int t_id = 0;
    while (1) // for multiple clients
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        //sleep(12);

        fds[t_id] = newsockfd;
        pthread_t thread;
        if (pthread_create(&thread, NULL, &handleclient, (void *)&fds[t_id]) != 0)
        {
            error("ERROR creating thread");
        }
        t_id++;
    }
    free(fds);
    close(sockfd); // Close the server socket
    return 0;
}
