#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include "helper.h"
// #include "helper/circular_queue.h"

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1000
// Request Queue
// CircularQueue requestQueue;

int gradingQueue[BUFFER_SIZE];
int queueSize = 0;
int queueFront = 0;
int queueRear = 0;

pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queueNotEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t queueNotFull = PTHREAD_COND_INITIALIZER;
pthread_mutex_t fileLock = PTHREAD_MUTEX_INITIALIZER;
void enqueueGradingRequest(int newsockfd)
{
    pthread_mutex_lock(&queueMutex);
    while (queueSize == BUFFER_SIZE)
    {
        // Queue is full, wait for space
        pthread_cond_wait(&queueNotFull, &queueMutex);
    }
    gradingQueue[queueRear] = newsockfd;
    queueRear = (queueRear + 1) % BUFFER_SIZE;
    queueSize++;
    pthread_cond_signal(&queueNotEmpty);
    pthread_mutex_unlock(&queueMutex);
}

int dequeueGradingRequest()
{
    pthread_mutex_lock(&queueMutex);
    while (queueSize == 0)
    {
        // Queue is empty, wait for requests
        pthread_cond_wait(&queueNotEmpty, &queueMutex);
    }
    int request = gradingQueue[queueFront];
    queueFront = (queueFront + 1) % BUFFER_SIZE;
    queueSize--;
    pthread_cond_signal(&queueNotFull);
    pthread_mutex_unlock(&queueMutex);
    return request;
}

// Function to count and write queue size to a file
void *countQueueSize(void *arg)
{
    FILE *outputFile; // File to write queue size
    outputFile = fopen("logs/queue_size.log", "w");
    if (outputFile == NULL)
    {
        error("Failed to open the output file");
        return (void *)NULL;
    }
    while (1)
    {
        int size = queueSize;
        fprintf(outputFile, "%d\n", size);
        fflush(outputFile); // Flush the file buffer to ensure data is written immediately
        sleep(1);           // Sleep for 10 seconds
    }
}

// Function to append request ID and status to a status file
int writeStatusToFile(int requestID, char *status)
{
    // Open the file in append mode
    FILE *file = fopen("request_status.csv", "a");
    if (file == NULL)
    {
        error("Error opening file");
    }
    // Append request ID and status to the file
    fprintf(file, "%d,%s\n", requestID, status);
    // Close the file
    fclose(file);
    return 0;
}

// Function to update status in a status file
int updateStatusToFile(int requestID, char *newStatus)
{
    // Open the file in read mode
    FILE *file = fopen("request_status.csv", "r");
    if (file == NULL)
    {
        error("Error opening file");
    }

    // Create a temporary file to write updated content
    FILE *tempFile = fopen("temp_status.csv", "w");
    if (tempFile == NULL)
    {
        fclose(file);
        error("Error opening temp file");
    }

    // // Search for the request ID in the file and update the status
    char line[256]; // Adjust the size as needed
    int requestFound = 0;
    while (fgets(line, sizeof(line), file) != NULL)
    {
        // printf("%s\n", line);
        char *token = strtok(line, ",");
        // printf("%s\n", token);
        char s1[11];
        sprintf(s1, "%d", requestID);
        if (token != NULL && strcmp(token, s1) == 0)
        {
            // Found the request ID, update the status
            fprintf(tempFile, "%s,%s\n", s1, newStatus);
            requestFound = 1;
        }
        else
        {
            token = strtok(NULL, " ");
            // Copy the line as is to the temporary file
            fprintf(tempFile, "%s,%s", line, token);
        }
    }

    // Close the files
    fclose(file);
    fclose(tempFile);

    // Remove the original file
    if (remove("request_status.csv") != 0)
    {
        error("Error removing original file");
    }

    // Rename the temporary file to the original file
    if (rename("temp_status.csv", "request_status.csv") != 0)
    {
        error("Error renaming temp file");
    }
    if (requestFound)
    {
        return 0; // Update successful
    }
    else
    {
        return -1; // Request ID not found
    }
}

// Function to read status from a status file
char *readStatusFromFile(int requestID)
{
    // Open the file in read mode
    FILE *file = fopen("request_status.csv", "r");
    if (file == NULL)
    {
        error("Error opening file");
        return NULL;
    }

    // Search for the request ID in the file
    char line[256]; // Adjust the size as needed
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *token = strtok(line, ",");
        char s1[11];
        sprintf(s1, "%d", requestID);
        if (token != NULL && strcmp(token, s1) == 0)
        {
            // Found the request ID, retrieve the status
            token = strtok(NULL, ",");
            char *status = strdup(token); // Dynamically allocate memory for the status
            status[strcspn(status, "\n")] = '\0';
            fclose(file);
            return status; // Return the dynamically allocated status
        }
    }

    // Request ID not found
    fclose(file);
    return NULL; // Return NULL to indicate not found
}

// Function to read status from a status file
char *readRemarksFromFile(char *statusID)
{
    // Open the file in read mode
    FILE *file = fopen("remarks.txt", "r");
    if (file == NULL)
    {
        error("Error opening file");
        return NULL;
    }

    // Search for the request ID in the file
    char line[256]; // Adjust the size as needed
    while (fgets(line, sizeof(line), file) != NULL)
    {
        // printf("Status :: %s", statusID);
        char *token = strtok(line, ",");
        // printf("Token 1 :: %s\n", token);
        if (token != NULL && (strcmp(token, statusID) == 0))
        {
            // Found the request ID, retrieve the status
            token = strtok(NULL, ",");
            // printf("Token 2 :: %s", token);
            char *remarks = strdup(token); // Dynamically allocate memory for the status
            // remarks[strcspn(remarks, "\n")] = '\0';
            fclose(file);
            return remarks; // Return the dynamically allocated status
        }
    }

    // Request ID not found
    fclose(file);
    return NULL; // Return NULL to indicate not found
}

int faultTolerance()
{
    printf("RUNNING FAULT TOLERANCE ::\n");
    // Open the file in read mode
    FILE *file = fopen("request_status.csv", "r");
    if (file == NULL)
    {
        error("Error Opening file");
    }
    // Search for the request ID in the file
    char line[256]; // Adjust the size as needed
    while (fgets(line, sizeof(line), file) != NULL)
    {
        // printf("%s\n", line);
        int requestID = atoi(strtok(line, ","));
        char *status = strtok(NULL, ",");
        // printf("%d :: %s\n", requestID, status);
        status[strcspn(status, "\n")] = '\0';
        if ((strcmp(status, "0") == 0) || (strcmp(status, "1") == 0))
        {
            enqueueGradingRequest(requestID);
            printf("Request ID = %d, is Re-Added to Queue.\n", requestID);
        }
    }
    printf("FAULT TOLERANCE DONE\n");
    return 0;
}

int grader(int requestID)
{
    char *programFileName = makeProgramFileName(requestID);
    char *execFileName = makeExecFileName(requestID);
    char *compileOutputFileName = makeCompileErrorFilename(requestID);
    char *runtimeOutputFileName = makeRuntimeErrorFilename(requestID);
    char *outputFileName = makeOutputFilename(requestID);
    char *outputDiffFileName = makeOutputDiffFilename(requestID);

    char *compileCommand = compile_command(requestID, programFileName, execFileName);
    char *runCommand = run_command(requestID, execFileName);
    char *outputCheckCommand = output_check_command(requestID, outputFileName);

    if (system(compileCommand) != 0)
    {
        pthread_mutex_lock(&fileLock);
        updateStatusToFile(requestID, "2");
        pthread_mutex_unlock(&fileLock);
    }
    else if (system(runCommand) != 0)
    {
        pthread_mutex_lock(&fileLock);
        updateStatusToFile(requestID, "3");
        pthread_mutex_unlock(&fileLock);
    }
    else
    {
        if (system(outputCheckCommand) != 0)
        {
            pthread_mutex_lock(&fileLock);
            updateStatusToFile(requestID, "4");
            pthread_mutex_unlock(&fileLock);
        }
        else
        {
            pthread_mutex_lock(&fileLock);
            updateStatusToFile(requestID, "5");
            pthread_mutex_unlock(&fileLock);
        }
    }

    free(programFileName);
    free(execFileName);
    free(compileOutputFileName);
    free(runtimeOutputFileName);
    free(outputFileName);
    free(outputDiffFileName);
    free(compileCommand);
    free(runCommand);
    free(outputCheckCommand);

    return 0;
}

void *handleClient(void *arg)
{
    while (1)
    {
        int requestID = dequeueGradingRequest();
        printf("Request ID=%d is assigned a Thread\n", requestID);

        pthread_mutex_lock(&fileLock);
        updateStatusToFile(requestID, "1");
        pthread_mutex_unlock(&fileLock);

        if (grader(requestID) == 0)
            printf("SUCCESS :: File Graded for Request ID = %d\n", requestID);
        else
            printf("ERROR :: File Cannot Be Graded for Request ID = %d\n", requestID);
    }
}

// Function to generate a unique 6-digit request ID based on current time
int generateUniqueRequestID()
{
    // Get current time
    time_t currentTime = time(NULL);

    // Use only the last 6 digits of the current time
    int timePart = (int)(currentTime % 1000000);

    // Generate a random 3-digit number
    int randomPart = rand() % 1000;

    // Combine the time and random parts to form a 9-digit request ID
    int requestID = timePart * 1000 + randomPart;

    return requestID;
}

int createNewRequest(int clientSockFD)
{
    int requestID = generateUniqueRequestID();
    int n;
    char *programFileName = makeProgramFileName(requestID);
    if (recv_file(clientSockFD, programFileName) != 0)
    {
        free(programFileName);
        error("ERROR :: FILE RECV ERROR");
    }
    free(programFileName);

    n = send(clientSockFD, "I got your code file for grading\n", 33, MSG_NOSIGNAL);
    if (n < 0)
        error("ERROR :: FILE SEND ERROR");

    enqueueGradingRequest(requestID);
    printf("Client with FD = %d is given Request ID = %d\n", clientSockFD, requestID);

    char requestIDString[30];
    // Convert integer to string
    sprintf(requestIDString, "Your RequestID is : %d\n", requestID);

    n = send(clientSockFD, requestIDString, strlen(requestIDString), MSG_NOSIGNAL);
    if (n < 0)
        error("ERROR :: FILE SEND ERROR");

    pthread_mutex_lock(&fileLock);
    n = writeStatusToFile(requestID, "0");
    pthread_mutex_unlock(&fileLock);
    if (n != 0)
    {
        error("ERROR :: File Write Error");
    }
    return 0;
}

int checkStatusRequest(int clientSockFD)
{
    int n;
    int requestID;
    n = recv(clientSockFD, &requestID, sizeof(requestID), 0);
    printf("Request id is %d\n", requestID);
    if (n < 0)
    {
        error("ERROR: RECV ERROR");
    }
    pthread_mutex_lock(&fileLock);
    char *status = readStatusFromFile(requestID);
    pthread_mutex_unlock(&fileLock);
    printf("The request ID is %d and status is: %s\n", requestID, status);
    if (status == NULL)
    {
        n = send(clientSockFD, "INVALID REQUEST ID", 20, MSG_NOSIGNAL);
    }
    else
    {
        // char *remarks = readRemarksFromFile(status);
        // printf("%s\n", remarks);
        // n = send(clientSockFD, remarks, strlen(remarks), MSG_NOSIGNAL);
        // if (n < 0)
        // {
        //     error("ERROR: SEND ERROR");
        // }
        printf("%s\n", status);
        if (strcmp(status, "2") == 0)
        {
            char *compileOutputFileName = makeCompileErrorFilename(requestID);
            n = send_file(clientSockFD, compileOutputFileName);
            free(compileOutputFileName);
        }
        else if (strcmp(status, "3") == 0)
        {
            char *runtimeOutputFileName = makeRuntimeErrorFilename(requestID);
            n = send_file(clientSockFD, runtimeOutputFileName);
            free(runtimeOutputFileName);
        }
        else if (strcmp(status, "4") == 0)
        {
            char *outputDiffFileName = makeOutputDiffFilename(requestID);
            n = send_file(clientSockFD, outputDiffFileName);
            free(outputDiffFileName);
        }
        else if (strcmp(status, "5") == 0)
        {
            if (send(clientSockFD,"Program ran successfully", 23, 0) == -1)
            {
                perror("Error sending file msg");
                return -1;
            }
        }
    }
    if (n < 0)
    {
        error("ERROR: SEND ERROR");
    }

    printf("Status Returned for Client with FD = %d with Request ID = %d\n", clientSockFD, requestID);

    return 0;
}

int getRequest(int clientSockFD)
{
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    // new or status_check
    int n = recv(clientSockFD, buffer, BUFFER_SIZE, 0);
    if (n < 0)
        return -1;
    if (strcmp(buffer, "new") == 0)
    {
        printf("going to create new request\n");
        sleep(1);
        return createNewRequest(clientSockFD);
    }
    else if (strcmp(buffer, "status") == 0)
    {
        printf("going to fetch requestID\n");
        sleep(1);
        return checkStatusRequest(clientSockFD);
    }
    return -1;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
        error("Usage: <portNumber> <threadPoolSize>");

    // Seed the random number generator
    srand((unsigned int)time(NULL));

    // Server and Client socket necessary variables
    int serverSockFD, serverPortNo;
    struct sockaddr_in serverAddr, clientAddr;
    int clientSockFD;

    // Make the server socket
    serverSockFD = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSockFD < 0)
        error("ERROR :: Socket Opening Failed");

    if (setsockopt(serverSockFD, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        error("ERROR :: setsockopt (SO_REUSEADDR) Failed");

    bzero((char *)&serverAddr, sizeof(serverAddr));
    serverPortNo = atoi(argv[1]);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(serverPortNo);

    // Get address size
    int clientAddrLen = sizeof(clientAddr);

    // Thread to count queue size
    // pthread_t queueCountThread;
    // pthread_create(&queueCountThread, NULL, countQueueSize, NULL);

    int threadPoolSize = atoi(argv[2]);
    pthread_t threads[threadPoolSize];

    // Initialize Request Queue
    // int requestQueueSize = atoi(argv[3]);
    // initQueue(&requestQueue, requestQueueSize);

    // if(faultTolerance()<0){
    //     error("ERROR :: While running fault tolerance");
    // }

    // Binding the server socket
    if (bind(serverSockFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        error("ERROR :: Socket Binding Failed");
    }
    printf("Server is Live on Port :: %d\n", serverPortNo);

    // Listening to the server socket
    if (listen(serverSockFD, MAX_CLIENTS) < 0)
    {
        error("ERROR :: Socket Listening Failed");
    }

    // Create thread pool
    for (int i = 0; i < threadPoolSize; i++)
    {
        if (pthread_create(&threads[i], NULL, handleClient, NULL) != 0)
        {
            close(serverSockFD);
            error("ERROR :: Thread creation failed");
        }
    }

    while (1)
    {
        clientSockFD = accept(serverSockFD, (struct sockaddr *)&clientAddr, &clientAddrLen);

        // If accept fails
        if (clientSockFD < 0)
        {
            printf("ERROR :: Client Socket Accept Failed");
        }

        printf("Accepted Client Connection From %s with FD = %d\n", inet_ntoa(clientAddr.sin_addr), clientSockFD);

        getRequest(clientSockFD);

        close(clientSockFD);
    }
    close(serverSockFD);

    return 0;
}
