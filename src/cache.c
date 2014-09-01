#include "cache.h"

pthread_mutex_t c_map_mutex;

int _insert_c(CacheLinkedList *list, char *key, unsigned int keySize, unsigned int entrySize) {
	LOG_INFO("Cache: _Insert\n");
	
	CacheLLElement *element = (CacheLLElement *) malloc(sizeof(CacheLLElement));
	if (element == NULL) {
		LOG_ERROR("Cache: NO_MEMORY_ERROR\n");
		return NO_MEMORY_ERROR;
	}
	
	element->key = key;
	element->keySize = keySize;
	element->entrySize = entrySize;
	
	if (list->size == 0) {
		list->head = element;
	} else {
		list->tail->next = element;
	}
	element->next = NULL;
	list->tail = element;
	
	list->size++;
	
	return OK;
}

int init_cache(Cache *cache, size_t maxSize) {
	LOG_INFO("Cache: InitCache\n");
	
	if (cache == NULL) {
		LOG_ERROR("Cache: CACHE_ERROR\n");
		return CACHE_ERROR;
	}
	
	cache->maxSize = maxSize;
	cache->curSize = 0;
	cache->keys.size = 0;
	
	if (init_hashmap(&cache->reqToresp, 23, 11, free) != OK) {
		return HASHMAP_ERROR;
	}
	
	pthread_mutex_init(&c_map_mutex, NULL);
	
	return OK;
}

int put_response_cache(Cache *cache, char *request, unsigned int reqLen, char *response, unsigned int resLen) {
	LOG_INFO("Cache: Put Response Cache\n");
	
	LOG_INFO("Cache:\n\tRequest: %s [%d]\n\tResponse: %s [%d]\n", request, reqLen, response, resLen);
	
	if (cache == NULL || request == NULL || reqLen == 0 || response == NULL || resLen == 0) {
		LOG_ERROR("Cache: CACHE ERROR\n");
		return CACHE_ERROR;
	}
	
	int ret;
	pthread_mutex_lock(&c_map_mutex);
	
	int newEntryTotalBytes = reqLen + resLen + (sizeof(unsigned int) << 1);
	if (newEntryTotalBytes + cache->curSize > cache->maxSize) {
		LOG_INFO("Cache: Remove entry\n");
		CacheLLElement *head = cache->keys.head;
				
		if (remove_hashmap(&cache->reqToresp, head->key, head->keySize) != OK) {
			ret = HASHMAP_ERROR;
			goto RETURN;
		}

		cache->keys.head = head->next;
		cache->keys.size--;
		cache->curSize -= head->entrySize;
		free(head);
	}
	
	cache->curSize += newEntryTotalBytes; 
	
	if (put_hashmap(&cache->reqToresp, request, reqLen, response, resLen) != OK) {
		return HASHMAP_ERROR;
	}
	
	char *key =  get_ref_key_hashmap(&cache->reqToresp, request, reqLen);
	if (key == NULL) {
		ret = HASHMAP_ERROR;
		goto RETURN;
	}
	
	if (_insert_c(&cache->keys, key, reqLen, newEntryTotalBytes) != OK) {
		ret = CACHE_LINKEDLIST_ERROR;
		goto RETURN;
	}
	ret = OK;
	
RETURN:	
	pthread_mutex_unlock(&c_map_mutex);
	
	return ;
}

int update_cache(Cache *cache, char *key, unsigned int keySize) {
	LOG_INFO("Cache: Update_Cache\n");
	
	if (cache == NULL || key == NULL) {
		LOG_ERROR("Cache: CACHE_ERROR\n");
		return CACHE_ERROR;
	}
	
	CacheLLElement *iter, *prev;
	for (iter = cache->keys.head, prev = NULL; iter != NULL; prev = iter, iter = iter->next) {
		if (strncmp(iter->key, key, keySize) == 0 && iter->keySize == keySize) {
			if (prev == NULL) {
				cache->keys.head = iter->next;
			} 
			else if (iter != cache->keys.tail) {
				prev->next = iter->next;
			}
			
			cache->keys.tail->next = iter;
			cache->keys.tail = iter;
			iter->next = NULL;
						
			return OK;
		}
	}
	
	return NOT_FOUND;
}

char* get_response_cache(Cache *cache, char *key, unsigned int keySize) {
	LOG_INFO("Cache: Get Response Cache\n");
	
	LOG_INFO("Cache:\n\tRequest: %s [%d]\n", key, keySize);
	pthread_mutex_lock(&c_map_mutex);
	char *rep = get_hashmap(&cache->reqToresp, key, keySize);
	pthread_mutex_unlock(&c_map_mutex);
	
	return rep;
}

int contains_request_cache(Cache *cache, char *key, unsigned int keySize) {
	LOG_INFO("Cache: Contains Request Cache\n");
	
	return contains_hashmap(&cache->reqToresp, key, keySize);
}

void free_cache(Cache *cache) {
	free_hashmap(&cache->reqToresp);
	
	CacheLLElement *iter, *temp;
	for (iter = cache->keys.head; iter != NULL; ) {
		temp = iter;
		iter = iter->next;
		free(temp);
	}	
}
/*
int main() {
	Cache c;
	
	init_cache(&c, 128);
	char buffer[3];
	int i;
	for (i = 0; i < 10; i++) {
		snprintf(buffer, 3, "%d", i);
		//printf("%s\n", buffer);
		put_response_cache(&c, buffer, strlen(buffer), "aek212121", 9);
		printf("\tCurrent Size: %d\n", c.curSize);
	}
	
	update_cache(&c, "5", 1);
}*/
