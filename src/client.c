#include "client.h"
#include "consumer.h"

pthread_mutex_t array_mutex;
pthread_cond_t array_cond;
pthread_attr_t cl_attr;

pthread_t client;

ClientInfo clInfo;

void* run_client(void *);

int shrink_array() {
	LOG_INFO("Client: Shrink array\n");
	int newSize;
	int i;
	int j = 0;
	
	newSize = clInfo.totalSize / 2;
	
	int *newArray = (int *) malloc(sizeof(int) * newSize);
	if (newArray == NULL) {
		LOG_ERROR("Client: NO_MEMORY_ERROR\n");
		return NO_MEMORY_ERROR;
	}
	
	for (i = 0; i < clInfo.totalSize; i++) {
		if (clInfo.arrayFD[i] != -1) {
			newArray[j] = clInfo.arrayFD[i];
		}
	}
	
	free(clInfo.arrayFD);
	clInfo.arrayFD = newArray;
	clInfo.maxSize = newSize * 0.75;
	clInfo.totalSize = newSize;
	
	return OK;
}

int expand_array() {
	LOG_INFO("Client: Expand Array\n");
	int newSize;
	
	newSize = 2 * clInfo.totalSize;
	
	int *newArrayFD = (int *) realloc(clInfo.arrayFD, newSize * sizeof(int));
	if (newArrayFD == NULL) {
		LOG_ERROR("Client: NO_MEMORY_ERROR\n");
		return NO_MEMORY_ERROR;
	}
	
	clInfo.arrayFD = newArrayFD;
	clInfo.maxSize = newSize * 0.75;
	clInfo.totalSize = newSize;
	
	return OK;
}

int setupClient(int size) {
	
	LOG_INFO("Client: Setup Client\n");
	
	if (size <= 0) {
		LOG_ERROR("Client: CLIENT_ERROR 0\n");
		return CLIENT_ERROR;
	}
	
	clInfo.thresHold = 0.75;
	clInfo.arrayFD = (int *) malloc(sizeof(int) * size);
	if (clInfo.arrayFD == NULL) {
		LOG_ERROR("Client: NO_MEMORY_ERROR\n");
		return NO_MEMORY_ERROR;
	}
	memset(clInfo.arrayFD, 0, sizeof(int) * size);
	clInfo.maxSize = clInfo.thresHold * size;
	clInfo.curSize = 0;
	clInfo.totalSize = size;
	
	pthread_cond_init(&array_cond, NULL);
	pthread_mutex_init(&array_mutex, NULL);
	pthread_attr_setdetachstate(&cl_attr, PTHREAD_CREATE_JOINABLE);
	
	if (pthread_create(&client, &cl_attr, run_client, NULL) < 0) {
		LOG_ERROR("Client: CLIENT_ERROR 1\n");
		free(clInfo.arrayFD);
		return CLIENT_ERROR;
	}
	
	LOG_INFO("Client: Return\n");
	
	return OK;
}

int insertFD(int fd) {
	pthread_mutex_lock(&array_mutex);
	
	LOG_INFO("Client: Insert FD\n");
	
	if ((clInfo.curSize + 1) > clInfo.maxSize) {
		if (!expand_array()) {
			pthread_mutex_unlock(&array_mutex);
			return CLIENT_ERROR;
		}
	}
	
	LOG_INFO("\tClient: Insert %d at %d.\n", fd, clInfo.curSize);
	clInfo.arrayFD[clInfo.curSize] = fd;
	clInfo.curSize++;
	pthread_cond_signal(&array_cond);
	pthread_mutex_unlock(&array_mutex);
	
	return OK;
}

void clear_client() {
	pthread_mutex_destroy(&array_mutex);
    pthread_cond_destroy(&array_cond);
    pthread_attr_destroy(&cl_attr);
    
    
    free(clInfo.arrayFD);
}

void* run_client(void *arg) {
	
	fd_set fdSet;
	int maxFD;
	int i;
	int ret;
	int curSize;
	socklen_t addrLen = sizeof(struct sockaddr_in);
	struct timeval tv;
	
	tv.tv_sec = 180;
	tv.tv_usec = 0;
	
AGAIN:
	pthread_mutex_lock(&array_mutex);
	while (clInfo.curSize == 0) {
		LOG_INFO("Client: Waiting\n");
		pthread_cond_wait(&array_cond, &array_mutex);
	}
	pthread_mutex_unlock(&array_mutex);
	
	maxFD = -1;
	FD_ZERO(&fdSet);
	
	pthread_mutex_lock(&array_mutex);
	
	curSize = clInfo.curSize;
	for (i = 0; i < clInfo.maxSize; i++) {
		if (clInfo.arrayFD[i] != -1) {
			if (clInfo.arrayFD[i] > maxFD) {
				maxFD = clInfo.arrayFD[i];
			}
			FD_SET(clInfo.arrayFD[i], &fdSet);
		}
	}
	pthread_mutex_unlock(&array_mutex);
	
	if ((ret = select(maxFD + 1, &fdSet, NULL, NULL, &tv)) < 0) {
		goto AGAIN;
	} else if (ret == 0) {
		pthread_mutex_lock(&array_mutex);
		for (i = 0; i < clInfo.maxSize; i++) {
			if (clInfo.arrayFD[i] != -1) {
				sendRequestTimeoutMsgToClients(clInfo.arrayFD[i]);
				clInfo.arrayFD[i] = -1;
				clInfo.curSize--;
			}
		}
		pthread_mutex_unlock(&array_mutex);
	} else {
	
		pthread_mutex_lock(&array_mutex);
		for (i = 0; i < clInfo.maxSize; i++) {
			if (FD_ISSET(clInfo.arrayFD[i], &fdSet)) {
				LOG_INFO("\tClient: %d is ready.\n", clInfo.arrayFD[i]);
				if (insertSock_webservers(clInfo.arrayFD[i]) != OK) {
					LOG_ERROR("Client: An error has occurred while trying to run the consumer thread\n");
				}
				
				clInfo.arrayFD[i] = -1;
				clInfo.curSize--;
			}
		}
		pthread_mutex_unlock(&array_mutex);
	}
	
	goto AGAIN;
	
RETURN:
	return NULL;
}
