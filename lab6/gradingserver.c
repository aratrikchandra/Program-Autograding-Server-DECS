#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

void error(char *msg) {
    perror(msg);
    exit(1);
}

// Function to compile and execute the submitted code
int compileAndExecute(char *sourceCode) {
    // Create temporary source file
    FILE *sourceFile = fopen("temp.c", "w");
    if (sourceFile == NULL) {
        perror("Error creating temporary source file");
        return 1; // COMPILER ERROR
    }
    fprintf(sourceFile, "%s", sourceCode);
    fclose(sourceFile);

    // Compile the code
    int compileStatus = system("gcc -o temp temp.c 2> compile_error.txt");

    if (compileStatus != 0) {
        return 1; // COMPILER ERROR
    }

    // Execute the compiled code
    int executionStatus = system("./temp > program_output.txt 2> runtime_error.txt");

    if (executionStatus != 0) {
        return 2; // RUNTIME ERROR
    }

    // Read the program's output
    FILE *outputFile = fopen("program_output.txt", "r");
    if (outputFile == NULL) {
        perror("Error opening program output file");
        return 3; // OUTPUT ERROR
    }

    char outputBuffer[256];
    char desiredOutput[] = "1 2 3 4 5 6 7 8 9 10";

    FILE *desiredFile = fopen("desired_output.txt", "w");
    if (desiredFile == NULL) {
        perror("Error creating temporary desired output file");
        return -1;
    }
    fprintf(desiredFile, "%s", desiredOutput);
    fclose(desiredFile);
    
    if (fgets(outputBuffer, sizeof(outputBuffer), outputFile) == NULL) {
        fclose(outputFile);
        return 3; // OUTPUT ERROR
    }

    fclose(outputFile);

    // Compare the program's output with desired output
    if (strcmp(outputBuffer, desiredOutput) != 0) {
        // Create a diff file to show the difference
        int diffStatus = system("diff -u program_output.txt desired_output.txt > diff.txt");
        return 3; // OUTPUT ERROR
    }

    return 0; // PASS
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    if (argc < 2) {
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

    listen(sockfd, 1);

    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        bzero(buffer, 256);
        n = read(newsockfd, buffer, 255);
        if (n < 0)
            error("ERROR reading from socket");
        buffer[n] = '\0'; // Add null-terminator
        printf("Received source code:\n%s", buffer);

        // Compile and execute the code
        int result = compileAndExecute(buffer);

        // Send response based on the result
        if (result == 0) {
            n = write(newsockfd, "PASS", 4);
        } else if (result == 1) {
            // Send compiler error details
            char compileErrorBuffer[256];
            FILE *compileErrorFile = fopen("compile_error.txt", "r");
                if (compileErrorFile != NULL) {
            size_t bytesRead = fread(compileErrorBuffer, 1, sizeof(compileErrorBuffer) - 1, compileErrorFile);
            compileErrorBuffer[bytesRead] = '\0'; // Add null-terminator
            fclose(compileErrorFile);
            printf("%s\n", compileErrorBuffer);
            n = write(newsockfd, "COMPILER ERROR\n", 16);
            sleep(1); // Add a delay
            n = write(newsockfd, compileErrorBuffer, strlen(compileErrorBuffer));
           } else {
                n = write(newsockfd, "COMPILER ERROR (Details not available)\n", 39);
            }
        } else if (result == 2) {
            // Send runtime error details
            char runtimeErrorBuffer[256];
            FILE *runtimeErrorFile = fopen("runtime_error.txt", "r");
            if (runtimeErrorFile != NULL) {
                fread(runtimeErrorBuffer, 1, sizeof(runtimeErrorBuffer), runtimeErrorFile);
                fclose(runtimeErrorFile);
                n = write(newsockfd, "RUNTIME ERROR\n", 15);
                sleep(1);
                n= write(newsockfd, runtimeErrorBuffer, strlen(runtimeErrorBuffer));
            } else {
                n = write(newsockfd, "RUNTIME ERROR (Details not available)\n", 38);
            }
        } else if (result == 3) {
            // Send output error details
            char diffBuffer[256];
            FILE *diffFile = fopen("diff.txt", "r");
            if (diffFile != NULL) {
                fread(diffBuffer, 1, sizeof(diffBuffer), diffFile);
                fclose(diffFile);
                //printf("%s\n", diffBuffer);
                n = write(newsockfd, "OUTPUT ERROR\n", 14);
                sleep(1);
                n = write(newsockfd, diffBuffer, strlen(diffBuffer));
            } else {
                n = write(newsockfd, "OUTPUT ERROR (Details not available)\n", 37);
            }
        }

        if (n < 0)
            error("ERROR writing to socket");
        sleep(1);
        n = write(newsockfd, "END\n", 4);
        close(newsockfd); // Close the connection for this submission
    }

    close(sockfd); // Close the server socket
    return 0;
}
