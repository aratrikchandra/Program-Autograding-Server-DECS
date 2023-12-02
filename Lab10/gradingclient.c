#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define MAX_TRIES 5
#define MAX_FILE_SIZE_BYTES 100
struct sockaddr_in serv_addr;

// Define a struct to hold the arguments
struct ThreadArgs
{
    char *arg1;
    char *arg2;
};

void error(char *msg)
{
    perror(msg);
    exit(1);
}
// Utility Function to send a file of any size to the grading server
int send_file(int sockfd, char *file_path)
// Arguments: socket fd, file name (can include path)
{
    char buffer[BUFFER_SIZE];            // buffer to read  from  file
    bzero(buffer, BUFFER_SIZE);          // initialize buffer to all NULLs
    FILE *file = fopen(file_path, "rb"); // open the file for reading, get file descriptor
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }

    // for finding file size in bytes
    fseek(file, 0L, SEEK_END);
    int file_size = ftell(file);

    // Reset file descriptor to beginning of file
    fseek(file, 0L, SEEK_SET);

    // buffer to send file size to server
    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    // copy the bytes of the file size integer into the char buffer
    memcpy(file_size_bytes, &file_size, sizeof(file_size));

    // send file size to server, return -1 if error
    if ((write(sockfd, &file_size, sizeof(file_size))) == -1)
    {
        perror("Error sending file size");
        fclose(file);
        return -1;
    }

    // now send the source code file
    while (!feof(file)) // while not reached end of file
    {

        // read buffer from file
        size_t bytes_read = fread(buffer, 1, BUFFER_SIZE - 1, file);

        // send to server
        if (send(sockfd, buffer, bytes_read, 0) == -1)
        {
            perror("Error sending file data");
            fclose(file);
            return -1;
        }

        bzero(buffer, BUFFER_SIZE);
    }
    // close file
    fclose(file);
    return 0;
}
void *workerThread(void *arguments)
{
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    int tries = 0;
    while (tries < MAX_TRIES)
    {
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0)
            break;
        sleep(1);
        tries++;
    }
    if (tries == MAX_TRIES)
    {
        error("ERROR connecting to server");
    }
    char buffer[BUFFER_SIZE];
    // Typecast the void pointer back to the original struct type
    struct ThreadArgs *args = (struct ThreadArgs *)arguments;
    // Access the individual arguments within the struct

    char *req_mode = args->arg1;

    if (strcmp(req_mode, "new") == 0)
    {
        if (write(sockfd, "new", 3) < 0)
        {
            error("ERROR writing to socket");
        }
        sleep(1);
        // Send new grading request
        if (send_file(sockfd, args->arg2) != 0)
        {
            close(sockfd);
            error("ERROR sending file");
        }
        sleep(1);
        // Receive and print response
        size_t bytes_read;
        while (true)
        {
        bytes_read = read(sockfd, buffer, BUFFER_SIZE-1);
        if (bytes_read <= 0)
            break;
        write(STDOUT_FILENO, buffer, bytes_read);
        bzero(buffer, BUFFER_SIZE);
        printf("\n");

	    sleep(1);
	    bytes_read = read(sockfd, buffer, BUFFER_SIZE-1);
	    if (bytes_read <=0)
		    break;
	    write(STDOUT_FILENO, buffer, bytes_read);
	    bzero(buffer, BUFFER_SIZE);
	    printf("\n");
        }
    }

    else if (strcmp(req_mode, "status") == 0)
    {
        if (write(sockfd, "status", 6) < 0)
        {
            error("ERROR writing to socket");
        }
        sleep(1);
        // Send request to check the status of a previous grading request
        int reqID = atoi(args->arg2);
        if (write(sockfd, &reqID, sizeof(int)) < 0)
        {
            error("ERROR writing to socket");
        }
        sleep(1);
        bzero(buffer, BUFFER_SIZE);
        // Receive and print response
        size_t bytes_read;
        char buffer[BUFFER_SIZE];

        /* handle large size requests in loop*/
        while (true)
        {
            bytes_read = read(sockfd, buffer, BUFFER_SIZE);
            if (bytes_read <= 0)
                break;
            
            write(STDOUT_FILENO, buffer, bytes_read);
            bzero(buffer, BUFFER_SIZE);
            printf("\n");
        }
    }

    close(sockfd);
    return (void *)0;
}
int main(int argc, char *argv[])
{
    int portno, n;
    struct hostent *server;

    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <new|status> <serverIP:port> <sourceCodeFileTobeGraded|requestID>\n", argv[0]);
        exit(1);
    }

    portno = atoi(strchr(argv[2], ':') + 1);
    char server_ip[256];
    strncpy(server_ip, argv[2], strchr(argv[2], ':') - argv[2]);
    server_ip[strchr(argv[2], ':') - argv[2]] = '\0';
    server = gethostbyname(server_ip);

    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr, server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(portno);

    // seggregating requests
    struct ThreadArgs args;

    // Assign values to the arguments
    args.arg1 = argv[1];
    args.arg2 = argv[3];

    pthread_t worker;

    pthread_create(&worker, NULL, workerThread, (void *)&args);

    pthread_join(worker, NULL);

    return 0;
}
