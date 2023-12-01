#ifndef GRADING_QUEUE_H
#define GRADING_QUEUE_H

#include <pthread.h>

#define BUFFER_SIZE 1024

extern pthread_mutex_t queueMutex;
extern pthread_cond_t queueNotEmpty;
extern pthread_cond_t queueNotFull;
extern int gradingQueue[BUFFER_SIZE];
extern int queueSize;
extern int queueFront;
extern int queueRear;

void enqueueGradingRequest(int newsockfd);
int dequeueGradingRequest();

#endif

