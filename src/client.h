#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "types.h"

typedef struct _ClientInfo {
	int *arrayFD;
	int maxSize;
	int curSize;
	int totalSize;
	double thresHold;
} ClientInfo;

int setupClient(int);

int insertFD(int);

void* run_client(void *);

void clear_client();

#endif
