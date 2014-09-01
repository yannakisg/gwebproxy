#include "consumer.h"
#include "queue.h"
#include "hashmap.h"
#include "types.h"
#include "http-parser-sender.h"

#include <time.h>

#define TOTAL_LISTS 23
#define MAX_LIST_SIZE 11

pthread_attr_t attr;

pthread_mutex_t queue_mutex;
pthread_cond_t queue_cond;

pthread_mutex_t wb_cl_map_mutex;
pthread_mutex_t savedReq_map_mutex;

pthread_t consumers[THREAD_POOL_SIZE];

Queue queue_clients;
Queue queue_webservers;
int hasPending;

void* consumerThread(void *);

HashMap wb_cl_map;
HashMap savedRequests;
HashMap cl_req_map;
Cache *cache;

int setupConsumer(Cache *c) {
	LOG_INFO("Consumer: Setup Consumer\n");
	int i;
	
	if (c == NULL || init_queue(&queue_clients) != OK || init_queue(&queue_webservers) != OK || init_hashmap(&wb_cl_map, TOTAL_LISTS, MAX_LIST_SIZE, free_queue) != OK 
			|| init_hashmap(&savedRequests, TOTAL_LISTS, MAX_LIST_SIZE, free) != OK || init_hashmap(&cl_req_map, TOTAL_LISTS, MAX_LIST_SIZE, free) != OK) {
		LOG_INFO("Consumer: CONSUMER_ERROR 0\n");
		return CONSUMER_ERROR;
	}
	
	cache = c;
	
	pthread_mutex_init(&wb_cl_map_mutex, NULL);
	pthread_mutex_init(&savedReq_map_mutex, NULL);
	pthread_mutex_init(&queue_mutex, NULL);
	pthread_cond_init(&queue_cond, NULL);
	
	hasPending = 0;
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	for (i = 0; i < THREAD_POOL_SIZE; i++) {
		if (pthread_create(&consumers[i], &attr, consumerThread, (void *)&i) < 0) {
			LOG_INFO("Consumer: Could not create thread %d.\n", i);
		}
		LOG_INFO("Consumer: Thread %d was created.\n", i);
		sleep(1);
	}
	
	return OK;
}

int contains_request(char *request, size_t len) {
	LOG_INFO("Consumer: Contains_Request\n");
	
	pthread_mutex_lock(&savedReq_map_mutex);
	int res = contains_hashmap(&savedRequests, request, len);
	pthread_mutex_unlock(&savedReq_map_mutex);
	
	return res;
}

int save_request(char *request, size_t len, int wSock, int clSock) {
	LOG_INFO("Consumer: Save_Request\n");
	
	int res;
	
	pthread_mutex_lock(&savedReq_map_mutex);
	res = put_hashmap(&savedRequests, request, len, &wSock, sizeof(int));
	if (res == OK) {
		res = put_hashmap(&cl_req_map, &clSock, sizeof(int), request, len);
	}
	pthread_mutex_unlock(&savedReq_map_mutex);
	
	return res;
}

int remove_request(int sock, int clSock) {
	LOG_INFO("Consumer: Remove_Request\n");
	int res;
	
	pthread_mutex_lock(&savedReq_map_mutex);
	res = find_and_remove_hashmap(&savedRequests, &sock, sizeof(int));
	if (res == OK) {
		res = remove_hashmap(&cl_req_map, &clSock, sizeof(int));
	}
	pthread_mutex_unlock(&savedReq_map_mutex);
	
	return res;
}

int get_request(char *request, size_t len) {
	LOG_INFO("Consumer: Remove_Request\n");
	
	pthread_mutex_lock(&savedReq_map_mutex);
	int sock = *((int *)get_hashmap(&savedRequests, request, len));
	pthread_mutex_unlock(&savedReq_map_mutex);
	
	return sock;
}

Queue* find_appropriate_clients(int wbSock) {
	LOG_INFO("Consumer: Find_Appropriate_Clients\n");
	
	pthread_mutex_lock(&wb_cl_map_mutex);
	Queue *clients = (Queue *)get_hashmap(&wb_cl_map, &wbSock, sizeof(int));
	pthread_mutex_unlock(&wb_cl_map_mutex);
	
	return clients;
}

int contains_appropriate_clients(int wbSock) {
	LOG_INFO("Consumer: Contains_Appropriate_Clients\n");
	
	pthread_mutex_lock(&wb_cl_map_mutex);
	int res = contains_hashmap(&wb_cl_map, &wbSock, sizeof(int));
	pthread_mutex_unlock(&wb_cl_map_mutex);
	
	return res;
}

int map_client_with_webserver(int wbSock, int clSock) {
	LOG_INFO("Consumer: Map_Client_With_Webserver\n");
	int res;
	
	if (contains_appropriate_clients(wbSock) != FOUND) {
		Queue queue;	
		init_queue(&queue);
		
		if (push_queue(&queue, &clSock) != OK) {
			return QUEUE_ERROR;
		}
		
		pthread_mutex_lock(&wb_cl_map_mutex);	
		res = put_hashmap(&wb_cl_map, &wbSock, sizeof(int), &queue, sizeof(Queue));
		pthread_mutex_unlock(&wb_cl_map_mutex);
	} else {
		Queue *queue = find_appropriate_clients(wbSock);
		
		pthread_mutex_lock(&wb_cl_map_mutex);	
		res = push_queue(queue, &clSock);
		pthread_mutex_unlock(&wb_cl_map_mutex);
	}
	
	return res;
}

int remove_appropiate_clients(int wbSock) {
	LOG_INFO("Consumer: Find_Appropriate_Client\n");
	
	pthread_mutex_lock(&wb_cl_map_mutex);
	int res = remove_hashmap(&wb_cl_map, &wbSock, sizeof(int));
	pthread_mutex_unlock(&wb_cl_map_mutex);
	
	return res;
}

int insertSock_clients(int sock) {
	LOG_INFO("Consumer: InsertSock to Clients\n");
	pthread_mutex_lock(&queue_mutex);
	if (push_queue(&queue_clients, &sock) != OK) {
		LOG_ERROR("Consumer: Unable to insert sock to Clients\n");
		pthread_mutex_unlock(&queue_mutex);
		return CONSUMER_ERROR;
	}
	
	hasPending++;
	pthread_cond_signal(&queue_cond);
	pthread_mutex_unlock(&queue_mutex);
	
	return OK;
}

int insertSock_webservers(int sock) {
	LOG_INFO("Consumer: InsertSock to Webservers\n");
	pthread_mutex_lock(&queue_mutex);
	if (push_queue(&queue_webservers, &sock) != OK) {
		LOG_ERROR("Consumer: Unable to insert sock to Webservers\n");
		pthread_mutex_unlock(&queue_mutex);
		return CONSUMER_ERROR;
	}
	
	hasPending++;
	pthread_cond_signal(&queue_cond);
	pthread_mutex_unlock(&queue_mutex);
	
	return OK;
}

void clear_consumer() {
	free_hashmap(&savedRequests);
	free_hashmap(&wb_cl_map);
	free_hashmap(&cl_req_map);
	free_cache(cache);
	free_queue(&queue_clients);
	free_queue(&queue_webservers);
	
	pthread_attr_destroy(&attr);
		
	pthread_cond_destroy(&queue_cond);
	pthread_mutex_destroy(&queue_mutex);
	pthread_mutex_destroy(&wb_cl_map_mutex);
	pthread_mutex_destroy(&savedReq_map_mutex);
}

void sendRequestTimeoutMsgToClients(int wSock) {
	Queue* clSocks = find_appropriate_clients(wSock);
	if (clSocks == NULL) {
		LOG_ERROR("Consumer: Could not find the appropriate clients.\n");
		return;
	}
	
	sendRequestTimeout(clSocks);
	
	if (remove_appropiate_clients(wSock) != OK) {
		LOG_ERROR("Consumer: Could not remove the appropriate clients from the hash map\n");
	}
}

void* consumerThread(void *args) {
	int id = *((int *)args);
	double random;
	struct drand48_data randBuffer;
	int sock;
	QElement *elmnt;
	ssize_t rcvBytes;
	int type;
	int ret;
	HttpGet httpGet;
	ssize_t totalSize;	
	char *savedBuffer;
	
	int lenBuffer = sizeof(char) * BUFSIZE;
	char *buffer;
	
	srand48_r(time(NULL), &randBuffer);
	
	while (1) {
		totalSize = 0;
		
		pthread_mutex_lock(&queue_mutex);
		while (hasPending == 0) {
			LOG_INFO("Consumer[%d] is waiting.\n", id);
			pthread_cond_wait(&queue_cond, &queue_mutex);
		}
		
		if (queue_clients.size > 0 && queue_webservers.size > 0) {
			drand48_r(&randBuffer, &random);
			if (random < 0.5) {
				elmnt = pop_queue(&queue_clients);
			} else {
				elmnt = pop_queue(&queue_webservers);
			}
		} else if (queue_clients.size > 0) {
			elmnt = pop_queue(&queue_clients);
		} else {
			elmnt = pop_queue(&queue_webservers);
		}
		
		hasPending--;
		if (hasPending > 0) {
			pthread_cond_signal(&queue_cond);
		}
		
		pthread_mutex_unlock(&queue_mutex);
		
		buffer = (char *) malloc(lenBuffer);
		if (buffer == NULL) {
			LOG_ERROR("Not enough free memory space\n");
			return NULL;
		}
		
		sock = elmnt->clSock;
		free(elmnt);

		rcvBytes = recv(sock, buffer, lenBuffer, 0);
		if (rcvBytes < 0) {
			LOG_ERROR("Consumer[%d]: An error occurred while receiving data from the client.\n", id);
			continue;
		} else if (rcvBytes == 0) {
			LOG_ERROR("Consumer[%d]: The client has performed a shutdown.\n", id);
			continue;
		}
		totalSize += rcvBytes;
		
		type = findHttpMessageType(buffer, rcvBytes);
		if (type == HTTP_GET) {
			int webSock;
			LOG_INFO("Consumer[%d]: Get Message received\n", id);
			
			ret = parseHttpGetMsg(buffer, rcvBytes, &httpGet);
			
			if (ret == HTTP_PARSER_ERROR || ret == NO_MEMORY_ERROR) {
				sendHttpBadReqMsg(sock);
			} else if (ret == HTTP_HOST_NOT_FOUND){
				sendHttpNotFoundMsg(sock);
			} else if (ret == OK) {
				char* response = get_response_cache(cache, httpGet.request, httpGet.reqLen);
				if (response != NULL) {
					LOG_INFO("\tConsumer[%d]: Cache Hit\n", id);
					
					update_cache(cache, httpGet.request, httpGet.reqLen);
					
					ret = sendSingleHttpResponseMsg(response, strlen(response), sock);
					if (ret != OK) {
						LOG_ERROR("Consumer[%d]: Unable to send the response Message\n", id);
					}
				} else {
					LOG_INFO("\tConsumer[%d]: No Cache Hit\n", id);
					
					if (contains_request(httpGet.request, httpGet.reqLen) != FOUND) {
						LOG_INFO("\tConsumer[%d]: Does not contain that request\n", id);								
						webSock = sendHttpGetMsgToServer(&httpGet);
						if (webSock < 0) {
							LOG_ERROR("Consumer[%d]: Did not send the HttpGetMessage to the Webserver\n", id);
						} else {
							if (save_request(httpGet.request, httpGet.reqLen, webSock, sock) != OK) {
								LOG_ERROR("Consumer[%d]: An error occurred while saving the request\n", id);
							}
							if (insertFD(webSock) != OK) {
								LOG_ERROR("Consumer[%d]: An error occurred while saving the socket\n", id);
							}
						}
					} else {
						LOG_INFO("\tConsumer[%d]: Does contain that request\n", id);	
						webSock = get_request(httpGet.request, httpGet.reqLen);
					}
					
					if (map_client_with_webserver(webSock, sock) != OK) {
						LOG_ERROR("Consumer[%d]: An error occurred while making the mapping between client's socket and webserver's one.\n", id);
					}
				}
				
				clear_httpGetMsg(&httpGet);
			}
		} else if (type == HTTP_RESPONSE) {
			LOG_INFO("Consumer[%d]: Response Message received\n", id);
			
			Queue* clSocks = find_appropriate_clients(sock);
			if (clSocks == NULL) {
				LOG_ERROR("Consumer[%d]: Could not find the appropriate clients.\n", id);
				continue;
			}
			
			savedBuffer = (char *) malloc(sizeof(char) * rcvBytes);
			if (savedBuffer == NULL) {
				LOG_ERROR("Consumer[%d]: Not enough free memory space.\n", id);
				continue;
			}
			
			memcpy(savedBuffer, buffer, rcvBytes);
			
			char *t;
			ssize_t prevSize = rcvBytes;
			memset(buffer, 0, lenBuffer);
			while ((rcvBytes = recv(sock, buffer, lenBuffer, 0)) != 0) {
				totalSize += rcvBytes;
				t = realloc(savedBuffer, totalSize);
				if (t == NULL) {
					LOG_ERROR("Consumer[%d]: Not enough free memory space.\n", id);
					break;
				}
				
				savedBuffer = t;
				memcpy(savedBuffer + prevSize, buffer, rcvBytes);
				prevSize = totalSize;
			}
			
			QElement *iter;
			char *req;
			int clSock = -1;
			for (iter = clSocks->head; iter != NULL; iter = iter->next) {
				req = get_hashmap(&cl_req_map, &iter->clSock, sizeof(int));
				if (req != NULL) {
					if (put_response_cache(cache, req, strlen(req) + 1, savedBuffer, totalSize) != OK) {
						LOG_ERROR("Consumer[%d]: Could not insert it in the cache\n", id);
					} 
					clSock = iter->clSock;
					break;
				}
			}
			
			ret = sendHttpResponseMsg(savedBuffer, totalSize, clSocks);
			if (ret != OK) {
				LOG_ERROR("Consumer[%d]: Unable to send the response Message\n", id);
			}
			
			
			if (remove_appropiate_clients(sock) != OK) {
				LOG_ERROR("Consumer[%d]: Could not remove the appropriate clients from the hash map\n", id);
			}
			if (remove_request(sock, clSock) != OK) {
				LOG_ERROR("Consumer[%d]: Could not remove the request from the hash map\n", id);
			}
			
			if (savedBuffer != NULL) {
				free(savedBuffer);
			}
			
		} else {
			LOG_INFO("Consumer[%d]: Unknown type of message\n", id);
			sendHttpBadReqMsg(sock);
		}
		
		free(buffer);
	}
	
	return NULL;
}


