#include "array.h"
#include "types.h"

/*
int binary_search(Array *array, int key, int min, int max) {
	
	while (max >= min) {
		int midpoint = ((max - min) / 2) + 1;	
		
		if (array->array[midpoint] < key) {
			min = midpoint + 1;
		} else if (array->array[midpoint] > key) {
			max = midpoint - 1;
		} else {
			return midpoint;
		}
	}
	
	return KEY_NOT_FOUND;
}*/

int init_array(Array *array, int size, int init) {
	LOG_INFO("Array: Init Array\n");
	if (array == NULL || size <= 0) {
		LOG_ERROR("Array: ARRAY_ERROR\n");
		return ARRAY_ERROR;
	}
	
	array->size = size;
	
	array->array = (int *) malloc(sizeof(int) * size);
	if (array->array == NULL) {
		LOG_ERROR("Array: NO_MEMORY_ERROR\n");
		return NO_MEMORY_ERROR;
	}
	
	int i;
	if (init) {
		for (i = 0; i < size; i++) {
			array->array[i] = 1;
		}
		array->isEmpty = 0;
		array->totalEmptySpaces = 0;
	} else {
		for (i = 0; i < size; i++) {
			array->array[i] = -1;
		}
		array->totalEmptySpaces = size;
		array->isEmpty = 1;
	}
	
	return OK;
}

void update_array(Array *array0, int pos, Array *array1) {
	LOG_INFO("Array: Update Array\n");
	array0->array[pos] = -1;
	array0->totalEmptySpaces++;
	if (array0->totalEmptySpaces == array0->size) {
		array0->isEmpty = 0;
	}
	
	array1->array[pos] = 1;
	array1->totalEmptySpaces--;
	array1->isEmpty = 0;
}

int headID_array(Array *array) {
	LOG_INFO("Array: HeadID Array\n");
	int i;
	for (i = 0; i < array->size; i++) {
		if (array->array[i] != -1) {
			return i;
		}
	}
	
	return -1;
}

void free_array(Array *array) {
	if (array == NULL) {
		return;
	}
	
	free(array->array);
	array->size = 0;
}
