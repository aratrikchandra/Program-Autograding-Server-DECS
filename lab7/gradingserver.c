#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

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
int compileAndExecute()
{

    // Compile the code
    int compileStatus = system("gcc -o temp temp.c 2> compile_error.txt");

    if (compileStatus != 0)
    {
        return 1; // COMPILER ERROR
    }

    // Execute the compiled code
    int executionStatus = system("./temp > program_output.txt 2> runtime_error.txt");

    if (executionStatus != 0)
    {
        return 2; // RUNTIME ERROR
    }

    // Read the program's output
    FILE *outputFile = fopen("program_output.txt", "r");
    if (outputFile == NULL)
    {
        perror("Error opening program output file");
        return 3; // OUTPUT ERROR
    }

    char outputBuffer[256];
    char desiredOutput[] = "1 2 3 4 5 6 7 8 9 10";

    FILE *desiredFile = fopen("desired_output.txt", "w");
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
        int diffStatus = system("diff -u program_output.txt desired_output.txt > diff.txt");
        return 3; // OUTPUT ERROR
    }

    return 0; // PASS
}

void action(int result, int newsockfd,int n)
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
                FILE *compileErrorFile = fopen("compile_error.txt", "r");
                if (compileErrorFile != NULL)
                {
                    size_t bytesRead = fread(compileErrorBuffer, 1, sizeof(compileErrorBuffer) - 1, compileErrorFile);
                    compileErrorBuffer[bytesRead] = '\0'; // Add null-terminator
                    fclose(compileErrorFile);
                    n = write(newsockfd, "COMPILER ERROR\n", 16);
                    usleep(100000); // 0.1 second delay
                    n = write(newsockfd, compileErrorBuffer, strlen(compileErrorBuffer));
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
                FILE *runtimeErrorFile = fopen("runtime_error.txt", "r");
                if (runtimeErrorFile != NULL)
                {
                    size_t bytesRead = fread(runtimeErrorBuffer, 1, sizeof(runtimeErrorBuffer) - 1, runtimeErrorFile);
                    runtimeErrorBuffer[bytesRead] = '\0'; // Add null-terminator
                    fclose(runtimeErrorFile);
                    n = write(newsockfd, "RUNTIME ERROR\n", 15);
                    usleep(100000); // 0.1 second delay
                    n = write(newsockfd, runtimeErrorBuffer, strlen(runtimeErrorBuffer));
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
                FILE *diffFile = fopen("diff.txt", "r");
                if (diffFile != NULL)
                {
                    size_t bytesRead = fread(diffBuffer, 1, sizeof(diffBuffer) - 1, diffFile);
                    diffBuffer[bytesRead] = '\0';
                    fclose(diffFile);
                    n = write(newsockfd, "OUTPUT ERROR\n", 14);
                    usleep(100000); // 0.1 second delay
                    n = write(newsockfd, diffBuffer, strlen(diffBuffer));
                }
                else
                {
                    n = write(newsockfd, "OUTPUT ERROR (Details not available)\n", 37);
                }

                remove("diff.txt");
            }

            if (n < 0)
                error("ERROR writing to socket");

}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

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

    while (1) 
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");
            bzero(buffer, 256);
            int length;
            n = read(newsockfd, &length, sizeof(int));
            FILE *sourceFile = fopen("temp.c", "w");
            if (sourceFile == NULL)
            {
                perror("Error creating temporary source file");
                return 1; 
            }
            while (length > 0)
            {
                bzero(buffer, 256);
                n = read(newsockfd, buffer, min(255, length));
                if (n > 0)
                {
                    fwrite(buffer, n, 1, sourceFile);
                    length -= n;
                }
                else if (n < 0)
                {
                    error("ERROR reading from socket");
                    break;
                }
            }

            printf("-->Received source code\n");
            fclose(sourceFile);
            // Compile and execute the code
            int result = compileAndExecute();

            action(result,newsockfd,n);

        close(newsockfd); // Close the connection for this client

        remove("compile_error.txt");
        remove("runtime_error.txt");
        remove("program_output.txt");
        remove("desired_output.txt");
        remove("temp.c");
        }
        close(sockfd); // Close the server socket
        return 0;
    }
