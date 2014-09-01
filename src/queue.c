#include "queue.h"


int init_queue(Queue *queue) {
	LOG_INFO ("Queue: Init Queue\n");
	queue->head = NULL;
	queue->tail = NULL;
	queue->size = 0;
	
	return OK;
}

int push_queue(Queue *queue, int *clSock) {
	LOG_INFO("Queue: Push Queue\n");
	if (clSock == NULL  || queue == NULL) {
		LOG_ERROR("Queue: QUEUE_ERROR\n");
		return QUEUE_ERROR;
	}
	
	QElement *element = (QElement *) malloc(sizeof(QElement));
	if (element == NULL) {
		LOG_ERROR("Queue: NO_MEMORY_ERROR\n");
		return NO_MEMORY_ERROR;
	}
	
	element->clSock = *clSock;
	
	if (queue->size == 0) {
		queue->head = element;
		queue->tail = element;
		element->prev = NULL;
		element->next = NULL;
	} else {
		QElement *prevTail = queue->tail;
		queue->tail = element;
		element->prev = prevTail;
		prevTail->next = element;
		element->next = NULL;
	}
	
	queue->size++;
	
	return OK;
}

QElement *pop_queue(Queue *queue) {
	LOG_INFO("Queue: Pop Queue\n");
	if (queue == NULL || queue->size == 0) {
		return NULL;
	}
	
	QElement *remElement = queue->head;
	
	queue->head = remElement->next;
	if (queue->head != NULL) {
		queue->head->prev = NULL;
	}
	
	queue->size--;
	return remElement;
	
}

void free_queue(void *q) {
	LOG_INFO("Queue: Free Queue\n");
	if (q == NULL) {
		return;
	}
	
	Queue *queue = (Queue *)q;
	
	QElement *element, *remElement;
	for (element = pop_queue(queue); element != NULL;) {
		remElement = element;
		element = element->next;
		free(remElement);
	}
	
	queue->size = 0;
	queue->head = NULL;
	queue->tail = NULL;
}
