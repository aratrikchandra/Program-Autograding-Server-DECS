#include <pthread.h>
#include "grading_queue.h"
int gradingQueue[BUFFER_SIZE];
int queueSize = 0;
int queueFront = 0;
int queueRear = 0;

pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queueNotEmpty = PTHREAD_COND_INITIALIZER;
pthread_cond_t queueNotFull = PTHREAD_COND_INITIALIZER;

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
