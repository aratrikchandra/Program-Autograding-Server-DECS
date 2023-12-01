#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <pthread.h>
#include "grading_queue.h"
#include "compiler.h"
#include "network.h"

#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 1024

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
   int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <port> <thread_pool_size>\n", argv[0]);
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

    listen(sockfd, MAX_CONNECTIONS);

    clilen = sizeof(cli_addr);

    // Create and initialize the thread pool
    int MAX_THREADS= atoi(argv[2]);
    pthread_t threadPool[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        if (pthread_create(&threadPool[i], NULL, workerThread, NULL) != 0) {
            error("ERROR creating thread");
        }
        printf("thread %d created\n", i);
    }

    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");
        // Enqueue the grading request
        enqueueGradingRequest(newsockfd);
    }

    for (int i = 0; i < MAX_THREADS; i++) {
        if (pthread_join(threadPool[i], NULL) != 0) {
            perror("Failed to join the thread");
        }
    }
    close(sockfd); // Close the server socket
    return 0;
}
