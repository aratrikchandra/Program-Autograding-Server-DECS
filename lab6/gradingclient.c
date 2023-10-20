#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

void error(char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <serverIP:port> <sourceCodeFileTobeGraded>\n", argv[0]);
        exit(1);
    }

    portno = atoi(strchr(argv[1], ':') + 1);
    char server_ip[256];
    strncpy(server_ip, argv[1], strchr(argv[1], ':') - argv[1]);
    server = gethostbyname(server_ip);

    if (server == NULL) {
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
    FILE *source_file = fopen(argv[2], "r");
    if (source_file == NULL)
        error("ERROR opening source code file");

    char buffer[256];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source_file)) > 0) {
        n = write(sockfd, buffer, bytes_read);
        if (n < 0)
            error("ERROR writing to socket");
    }

    fclose(source_file);

while (1) {
    // Receive and print response from server
    bzero(buffer, 256);
    n = read(sockfd, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");
    
    // Break loop if end of transmission signal is received
    if (strcmp(buffer, "END\n") == 0)
        break;

    printf("-->%s\n", buffer);
}


    close(sockfd);
    return 0;
}
