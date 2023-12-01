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
#include <time.h>
// #include "helper/circular_queue.h"

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1000
// Request Queue
// CircularQueue requestQueue;

struct node {
    int sockfd;
    int requestID;
};


int Queue[BUFFER_SIZE][2];
int queueSize = 0;
int queueFront = 0;
int queueRear = 0;

pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queueNotEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t queueNotFull = PTHREAD_COND_INITIALIZER;
pthread_mutex_t fileLock = PTHREAD_MUTEX_INITIALIZER;

int generateNewRequest(int clientSockFD, int requestID);


void enqueueRequest(int newsockfd, int requestID)
{
    pthread_mutex_lock(&queueMutex);
    while (queueSize == BUFFER_SIZE)
    {
        // Queue is full, wait for space
        pthread_cond_wait(&queueNotFull, &queueMutex);
    }
    Queue[queueRear][0] = newsockfd;
    Queue[queueRear][1] = requestID;
    queueRear = (queueRear + 1) % BUFFER_SIZE;
    queueSize++;
    pthread_cond_signal(&queueNotEmpty);
    pthread_mutex_unlock(&queueMutex);
}

struct node dequeueRequest()
{   struct node data;
    // sleep(10);
    srand(time(NULL));  // Seed the random number generator
    unsigned int microseconds = rand() % 1000000;  // Generate a random number between 0 and 999999
    usleep(microseconds);
    
    pthread_mutex_lock(&queueMutex);
    while (queueSize == 0)
    {
        // Queue is empty, wait for requests
        pthread_cond_wait(&queueNotEmpty, &queueMutex);
    }
    data.sockfd = Queue[queueFront][0];
    data.requestID = Queue[queueFront][1];
    
    queueFront = (queueFront + 1) % BUFFER_SIZE;
    queueSize--;
    pthread_cond_signal(&queueNotFull);
    pthread_mutex_unlock(&queueMutex);
    return data;
}

// Queue Position Function
int getQueuePos(int requestID){
    int pos = -1;
    for (int i = queueFront; i != queueRear+1; i = (i+1)%BUFFER_SIZE){
        pos++;
        if ( Queue[i][1] == requestID){
            return pos;
        }
    }
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
        // sleep(1);           // Sleep for 10 seconds
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
char *readRemarksFromFile(char *statusID, int reqID)
{

	char *remarks = (char *) malloc(200 * sizeof(char));
    if(strcmp(statusID, "0") == 0)
    {
        int queuePos = getQueuePos(reqID);
	    sprintf(remarks,"Your grading request ID %d has been accepted. It is currently at  position %d  in the queue.",reqID, queuePos);
	    return remarks;
    }
    else if(strcmp(statusID, "1")==0)
    {
	    sprintf(remarks, "Your grading request ID %d has been accepted and is currently being processed.", reqID);
	    return remarks;
    }
    else
    {
	    sprintf(remarks, "Your grading request ID %d processing is done, here are the results:",reqID);
	    return remarks;
    }
    // Request ID not found
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
    printf("\n\nQueue State:\n");
    while (fgets(line, sizeof(line), file) != NULL)
    {
        printf("\nQueue content: %s", line);
        int requestID = atoi(strtok(line, ","));
        char *status = strtok(NULL, ",");
        // printf("%d :: %s\n", requestID, status);
        status[strcspn(status, "\n")] = '\0';
        if ((strcmp(status, "0") == 0) || (strcmp(status, "1") == 0))
        {
            enqueueRequest(-1, requestID);
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
        struct node data = dequeueRequest();
        int requestID = data.requestID;
        int sockfd = data.sockfd;
        if (sockfd != -1){
            generateNewRequest(sockfd, requestID);
            enqueueRequest(-1, requestID);
            continue;
        }
        
        printf("Request ID = %d is assigned a Thread\n", requestID);

        pthread_mutex_lock(&fileLock);
        updateStatusToFile(requestID, "1");
        pthread_mutex_unlock(&fileLock);

        if (grader(requestID) == 0)
            printf("\nSUCCESS :: File Graded for Request ID = %d\n", requestID);
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

int generateNewRequest(int clientSockFD, int requestID)
{
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

    // enqueueRequest(requestID, clientSockFD);
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
        close(clientSockFD);
        error("ERROR :: File Write Error");        
    }
    close(clientSockFD);
    return 0;
}

void *checkStatusRequest(void *arg)
{
    int clientSockFD= *((int *)arg);
    // printf("\n in chk status, client sfd: %d", clientSockFD);
    int n;
    int requestID;
    // sleep(1);
    n = recv(clientSockFD, &requestID, sizeof(requestID), 0);
    printf("Request id is %d\n", requestID);
    if (n < 0)
    {   
        close(clientSockFD);
        error("ERROR: RECV ERROR");
    }
    pthread_mutex_lock(&fileLock);
    char *status = readStatusFromFile(requestID);
    pthread_mutex_unlock(&fileLock);
    printf("The request ID is %d and status is: %s\n", requestID, status);
    if (status == NULL)
    {   char msg[200];
        sprintf(msg, "Grading request %d not found. Please check and resend your request id", requestID);
        n = send(clientSockFD, msg, strlen(msg), MSG_NOSIGNAL);
    }
    else
    {
        char *remarks = readRemarksFromFile(status, requestID);
        printf("%s\n", remarks);
        n = send(clientSockFD, remarks, strlen(remarks), MSG_NOSIGNAL);
        if (n < 0)
        {
            close(clientSockFD);
            error("ERROR: SEND ERROR");
            //free((int *)arg);
        }
        printf("%s\n", status);
        if (strcmp(status, "2") == 0)
        {
            char *compileOutputFileName = makeCompileErrorFilename(requestID);
            n = send_file(clientSockFD, compileOutputFileName);
            free(compileOutputFileName);
            //free((int *)arg);
        }
        else if (strcmp(status, "3") == 0)
        {
            char *runtimeOutputFileName = makeRuntimeErrorFilename(requestID);
            n = send_file(clientSockFD, runtimeOutputFileName);
            free(runtimeOutputFileName);
            //free((int *)arg);
        }
        else if (strcmp(status, "4") == 0)
        {
            char *outputDiffFileName = makeOutputDiffFilename(requestID);
            n = send_file(clientSockFD, outputDiffFileName);
            free(outputDiffFileName);
            //free((int *)arg);
        }
        else if (strcmp(status, "5") == 0)
        {
            if (send(clientSockFD,"Program ran successfully", 23, 0) == -1)
            {
                close(clientSockFD);
                perror("Error sending file msg");
              //  free((int *)arg);
                return NULL;
            }
        }
    }
    if (n < 0)
    {
        close(clientSockFD);
        error("ERROR: SEND ERROR");
        //free((int *)arg);
    }

    printf("Status Returned for Client with FD = %d with Request ID = %d\n", clientSockFD, requestID);
    close(clientSockFD);
    free((int *)arg);
    return NULL;
}

int getRequest(int clientSockFD)
{
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    // new or status_check
    int n = recv(clientSockFD, buffer, BUFFER_SIZE, 0);
    printf("\ngetrequest n: %d\n", n);
    if (n <= 0){
        printf("\n\nrecv nothing, but conn accepted");
        return -1;
    }
    if (strcmp(buffer, "new") == 0)
    {
        printf("\ngoing to create new request\n");
        // sleep(1);
        // return generateNewRequest(clientSockFD);

        int requestID = generateUniqueRequestID();
        enqueueRequest(clientSockFD, requestID);
        return 0;
    }
    else if (strcmp(buffer, "status") == 0)
    {
        printf("going to fetch requestID\n");
        pthread_t thread;
        printf("\nsfd: %d", clientSockFD);
        // sleep(1);
        int* sockfd = (int*)malloc(sizeof(int));
        *sockfd = clientSockFD;
        int rc = pthread_create(&thread, NULL, checkStatusRequest, (void *)sockfd);
        assert(rc==0);
        pthread_detach(thread);
        return 0;
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
    
    system("touch request_status.csv");

    if(faultTolerance()<0){
        error("ERROR :: While running fault tolerance");
    }

    // Binding the server socket
    if (bind(serverSockFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        error("ERROR :: Socket Binding Failed");
    }
    printf("Server is Live on Port :: %d\n", serverPortNo);

    // Listening to the server socket
    if (listen(serverSockFD, SOMAXCONN) < 0)
    {
        error("ERROR :: Socket Listening Failed");
    }

    // Create thread pool
    for (int i = 0; i < threadPoolSize; i++)
    {
        // thread pool for handling new requests
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
        // sleep(1);
        // close(clientSockFD);
    }
    // close(serverSockFD);

    return 0;
}
