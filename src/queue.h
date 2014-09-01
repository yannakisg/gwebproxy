#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "cache.h"
#include "types.h"

typedef struct _QElement {
	int clSock;
	struct _QElement *next;
	struct _QElement *prev;
}QElement;

typedef struct _Queue {
	QElement *head;
	QElement *tail;
	unsigned int size;
} Queue;

int init_queue(Queue *);

int push_queue(Queue *, int *);

QElement *pop_queue(Queue *);

void free_queue(void *);

#endif
