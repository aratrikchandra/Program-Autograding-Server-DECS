#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/select.h> // Include the select header
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <errno.h>

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

    if (argc < 6)
    {
        fprintf(stderr, "Usage: %s <serverIP:port> <sourceCodeFileTobeGraded> <loopNum> <sleepTimeSeconds> <timeout-seconds>\n", argv[0]);
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

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr, server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(portno);

    double sum_response_time = 0;
    int successful_responses = 0;
    int numSentRequests = 0;
    int num_Errors = 0;
    int numTimeouts = 0;

//if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
       // error("ERROR connecting to server");

    //n = write(sockfd, &loopNum, sizeof(int));
    //if (n < 0)
    //{
      //  fprintf(stderr, "ERROR writing to socket");
        //num_Errors++;
    //}

    // Read source code from file
    struct timeval Tsend, Trecv;

    struct timeval Tstart, Tend;
    gettimeofday(&Tstart, NULL); // Get the time before starting the loop
    for (int i = 0; i < loopNum; i++)
    {
	    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	    if (sockfd < 0)
		    error("ERROR opening socket");
	    struct timeval timeout;
    timeout.tv_sec = atoi(argv[5]); // Timeout in seconds
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        error("ERROR setting socket option");

    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        error("ERROR setting socket option");
	    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		    error("ERROR connecting to server");
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
        if ((bytes_read = fread(buffer, 1, sizeof(buffer), source_file)) > 0)
        {
            int n = write(sockfd, buffer, bytes_read);
            if (n < 0)
            {
                fprintf(stderr, "ERROR writing to socket");
                num_Errors++;
            }
        }
        fclose(source_file);
        numSentRequests++;
        gettimeofday(&Tsend, NULL); // Get the time after sending the request

        // Data is available for reading, proceed to read and print response
        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);

        if (n <= 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                fprintf(stderr, "Timeout occurred\n");
                numTimeouts++;
                // int n = write(sockfd, "Timeout\n",8);
                // if (n < 0)
                // {
                //     fprintf(stderr, "ERROR writing to socket");
                //     num_Errors++;
                // }
                continue;
            }
            else
            {
                fprintf(stderr, "ERROR reading from socket");
                num_Errors++;
                continue;
            }
        }
	close(sockfd);

        printf("-->%s\n", buffer);

        gettimeofday(&Trecv, NULL); // Get the time after receiving the response

        successful_responses++;

        double response_time = (Trecv.tv_sec - Tsend.tv_sec) + (Trecv.tv_usec - Tsend.tv_usec) / 1000000.0;
        sum_response_time += response_time;

        sleep(sleepTimeSeconds); // Sleep for the specified amount of time
    }

    gettimeofday(&Tend, NULL); // Get the time after finishing the loop

    double total_time = (Tend.tv_sec - Tstart.tv_sec) + (Tend.tv_usec - Tstart.tv_usec) / 1000000.0;

    printf("No. of requests sent: %d\n", numSentRequests);
    printf("Number of successful responses: %d\n", successful_responses);
    printf("No. of timeouts: %d\n", numTimeouts);
    printf("No. of Erroenous requests responses: %d\n", num_Errors);
    printf("Total time taken to complete the loop: %.6f seconds\n", total_time);
    printf("Average response time: %.6f seconds\n", sum_response_time / successful_responses);

    return 0;
}
