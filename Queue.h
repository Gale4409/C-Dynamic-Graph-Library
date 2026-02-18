#ifndef QUEUE_H
#define QUEUE_H

typedef struct queue *Queue;

Queue Qinit(void);
void Qput(Queue q, int value);
int Qget(Queue q);
int QisEmpty(Queue q);
void Qfree(Queue q);

#endif