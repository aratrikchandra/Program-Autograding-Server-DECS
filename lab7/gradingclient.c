#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 5)
    {
        fprintf(stderr, "Usage: %s <serverIP:port> <sourceCodeFileTobeGraded> <loopNum> <sleepTimeSeconds>\n", argv[0]);
        exit(1);
    }

    int loopNum = atoi(argv[3]);
    int sleepTimeSeconds = atoi(argv[4]);

    portno = atoi(strchr(argv[1], ':') + 1);
    char server_ip[256];
    strncpy(server_ip, argv[1], strchr(argv[1], ':') - argv[1]);
    server = gethostbyname(server_ip);

    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr, server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting to server");

    // Read source code from file
 struct timeval Tsend, Trecv;
    double sum_response_time = 0;
    int successful_responses = 0;

struct timeval Tstart, Tend;
    gettimeofday(&Tstart, NULL); // Get the time before starting the loop
    for (int i = 0; i < loopNum; i++)
    {
        // Read source code from file
        FILE *source_file = fopen(argv[2], "r");
        if (source_file == NULL)
            error("ERROR opening source code file");

        char buffer[256];
        size_t bytes_read;
        fseek(source_file, 0L, SEEK_END);
        int length = ftell(source_file);
        fseek(source_file, 0L, SEEK_SET);
        write(sockfd, &length, sizeof(int));
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), source_file)) > 0)
        {
            int n = write(sockfd, buffer, bytes_read);
            if (n < 0)
                error("ERROR writing to socket");
        }
        fclose(source_file);

        gettimeofday(&Tsend, NULL); // Get the time before sending the request

        int flag=1;
        while (1)
        {
            // Receive and print response from server
            bzero(buffer, 256);
            n = read(sockfd, buffer, 255);
            if (n < 0)
            {
                error("ERROR reading from socket");
                flag=0;
            }
                

            // Break loop if end of transmission signal is received
            if (strcmp(buffer, "END\n") == 0)
                break;

            printf("-->%s\n", buffer);
        }

        gettimeofday(&Trecv, NULL); // Get the time after receiving the response

        double response_time = (Trecv.tv_sec - Tsend.tv_sec) + (Trecv.tv_usec - Tsend.tv_usec) / 1000000.0;
        sum_response_time += response_time;
        if(flag)
        successful_responses++;

        sleep(sleepTimeSeconds); // Sleep for the specified amount of time
    }

    gettimeofday(&Tend, NULL); // Get the time after finishing the loop

    double total_time = (Tend.tv_sec - Tstart.tv_sec) + (Tend.tv_usec - Tstart.tv_usec) / 1000000.0;
    printf("Total time taken to complete the loop: %.6f seconds\n", total_time);

    printf("Average response time: %.6f seconds\n", sum_response_time / successful_responses);
    printf("Number of successful responses: %d\n", successful_responses);

    close(sockfd);
    return 0;
}

