#ifndef _HASHMAP_H_
#define _HASHMAP_H_

typedef struct _LListElement {
	void *key;
	unsigned int keySize;
	void *data;
	unsigned int dataSize;
	struct _LListElement *next;
}LListElement;

typedef struct _LinkedList {
	LListElement *head;
	unsigned int size;
}LinkedList;

typedef struct _HashMap {
	unsigned int maxListSize;
	unsigned int totalLists;
	unsigned int size;
	LinkedList *arrayOfLists;
	void (*_free)(void *);
}HashMap;

int init_hashmap(HashMap *, unsigned int, unsigned int, void (*_f)(void *));

int put_hashmap(HashMap *, void *, unsigned int, void *, unsigned int);

void* get_hashmap(HashMap *, void *, unsigned int);

void* get_ref_key_hashmap(HashMap *, void *, unsigned int);

int remove_hashmap(HashMap *, void *, unsigned int);

int contains_hashmap(HashMap *, void *, unsigned int);

int find_and_remove_hashmap(HashMap *, void*, unsigned int);

void free_hashmap(HashMap *);

#endif
