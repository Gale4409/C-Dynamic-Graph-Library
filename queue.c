#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct QueueNode {
    int data;
    struct QueueNode* next;
} QueueNode;

struct queue {
    QueueNode *front, *rear;
};

Queue Qinit(void) {
    Queue q = malloc(sizeof(struct queue));
    if (q == NULL) return NULL;
    q->front = q->rear = NULL;
    return q;
}

void Qput(Queue q, int value) {
    if (q == NULL) return; // Sicurezza

    QueueNode* temp = malloc(sizeof(QueueNode));
    temp->data = value;
    temp->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
}

int Qget(Queue q) {
    if (q == NULL || q->front == NULL) return -1;

    QueueNode* temp = q->front;
    int data = temp->data;
    q->front = q->front->next;

    if (q->front == NULL) q->rear = NULL;
    free(temp);

    return data;
}

int QisEmpty(Queue q) {
    if (q == NULL) return 1;
    return q->front == NULL;
}

void Qfree(Queue q) {
    if (q == NULL) return;
    while (!QisEmpty(q)) {
        Qget(q);
    }
    free(q);
}