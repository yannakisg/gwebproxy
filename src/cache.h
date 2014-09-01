#ifndef _CACHE_H_
#define _CACHE_H_

#include "types.h"
#include "hashmap.h"

typedef struct _CacheLLElement {
	struct _CacheLLElement *next;
	char *key;
	unsigned int keySize;
	unsigned int entrySize;
} CacheLLElement;

typedef struct _CacheLinkedList {
	CacheLLElement *head;
	CacheLLElement *tail;
	size_t size;
}CacheLinkedList;

typedef struct _Cache {
	size_t maxSize;
	size_t curSize;
	HashMap reqToresp;
	CacheLinkedList keys;
} Cache;

int init_cache(Cache *, size_t);

int put_response_cache(Cache *, char *, unsigned int, char *, unsigned int);

int update_cache(Cache *, char *, unsigned int);

char* get_response_cache(Cache *, char *, unsigned int);

int contains_request_cache(Cache *, char *, unsigned int);

void free_cache(Cache *);

#endif
