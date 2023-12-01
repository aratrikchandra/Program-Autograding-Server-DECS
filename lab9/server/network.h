#ifndef NETWORK_H
#define NETWORK_H

void error(char *msg);
void action(int result, int newsockfd, int n, int tid);
void *workerThread(void *arg);

#endif
