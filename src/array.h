#ifndef _ARRAY_H_
#define _ARRAY_H_


typedef struct _Array {
	int *array;
	unsigned int size;
	int isEmpty;
	int totalEmptySpaces;
} Array;

// sort elements depending on the id

int init_array(Array *, int, int);

void free_array(Array *);

void update_array(Array *, int, Array *);

void update_stopped_array(Array *, int, Array *);

int headID_array(Array *);

#endif
