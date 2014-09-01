#include "hashmap.h"
#include "types.h"

void print(HashMap *hashMap) {
	unsigned int i;
	LListElement *iter;
	for (i = 0; i < hashMap->totalLists; i++) {
		printf("List[%d]\n", i);
		for (iter = hashMap->arrayOfLists[i].head; iter != NULL; iter = iter->next) {
			int *key = (int *) iter->key;
			char *str = (char *)iter->data;
			printf("\tKey => %d, Data => %s\n", *key, str);
		}
	}
}

unsigned long hash(void *value, uint32_t size) {
	LOG_INFO("HashMap: Hash\n");
	unsigned int hash = 2166136261UL;
	int c;
	unsigned int i;
	unsigned char *p = (uint8_t *)value;
	
	for (i = 0; i < size; i++) {
		hash =  (hash ^ p[i]) * 16777619;
	}
	LOG_INFO("HashMap: HashValue %u\n", hash);
	return hash;
}

void _putTo(LinkedList *list, LListElement *element) {
	LOG_INFO("HashMap: _PutTo\n");
	
	LListElement *prevHead = list->head;
	element->next = prevHead;
	list->head = element;
	list->size++;
}

int _resize(HashMap *hashMap) {
	LOG_INFO("HashMap: _Resize\n");
	
	unsigned int newTotalLists = (unsigned int) hashMap->totalLists * 2;
	
	LinkedList *newArrayOfLists = (LinkedList *)calloc(newTotalLists, sizeof(LinkedList));
	if (newArrayOfLists == NULL) {
		LOG_ERROR("HashMap: NO_MEMORY_ERROR 0\n");
		return NO_MEMORY_ERROR;
	}
	
	unsigned int i;
	unsigned int pos;
	LListElement *iter;
	LListElement *next;
	for (i = 0; i < hashMap->totalLists; i++) {
		for (iter = hashMap->arrayOfLists[i].head; iter != NULL;) {
			pos = hash(iter->key, iter->keySize) % newTotalLists;
			next = iter->next;
			_putTo(&newArrayOfLists[pos], iter);
			iter = next;
		}
	}
	
	LinkedList *prev = hashMap->arrayOfLists;
	hashMap->totalLists = newTotalLists;
	hashMap->arrayOfLists = newArrayOfLists;
	
	free(prev);
	
	return 1;
}

int _insert(HashMap *hashMap, unsigned int pos, void *key, unsigned int keySize, void *data, unsigned int dataSize) {
	LOG_INFO("HashMap: _Insert\n");
	
	LinkedList *list = &hashMap->arrayOfLists[pos];
	if ((list->size + 1) > hashMap->maxListSize) {
		if (_resize(hashMap) != OK) {
			LOG_ERROR("HashMap: HASHMAP_ERROR 0\n");
			return HASHMAP_ERROR;
		} else {
			pos = hash(key, keySize) % hashMap->totalLists;
			list = &hashMap->arrayOfLists[pos];
		}
	}
	
	LListElement *newElement = (LListElement *) calloc(1, sizeof(LListElement));
	if (newElement == NULL) {
		LOG_ERROR("HashMap: NO_MEMORY_ERROR 0\n");
		return NO_MEMORY_ERROR;
	}
	
	newElement->key = calloc(1, keySize);
	if (newElement->key == NULL) {
		free(newElement);
		LOG_ERROR("HashMap: HASHMAP_ERROR 1\n");
		return HASHMAP_ERROR;
	}
	
	newElement->data = calloc(1, dataSize);
	if (newElement->data == NULL) {
		free(newElement->key);
		free(newElement);
		LOG_ERROR("HashMap: HASHMAP_ERROR 2\n");
		return HASHMAP_ERROR;
	}
	newElement->dataSize = dataSize;
	newElement->keySize = keySize;
	
	memcpy(newElement->key, key, keySize);
	memcpy(newElement->data, data, dataSize);
	
	LListElement *prevHead = list->head;
	newElement->next = prevHead;
	list->head = newElement;
	list->size++;
	
	hashMap->size++;
	
	return OK;
}

void* _get(HashMap *hashMap, unsigned int pos, void *key, unsigned int keySize) {
	LOG_INFO("HashMap: _Get\n");
	
	LListElement *iter;
	for (iter = hashMap->arrayOfLists[pos].head; iter != NULL; iter = iter->next) {
		if (memcmp(iter->key, key, keySize) == 0) {
			return iter->data;
		}
	}
	
	return NULL;
}

void* _get_ref_key(HashMap *hashMap, unsigned int pos, void *key, unsigned int keySize) {
	LOG_INFO("HashMap: _Get_Ref_Key\n");
	
	LListElement *iter;
	for (iter = hashMap->arrayOfLists[pos].head; iter != NULL; iter = iter->next) {
		if (memcmp(iter->key, key, keySize) == 0) {
			return iter->key;
		}
	}
	
	return NULL;
}

int _contains(HashMap *hashMap, unsigned int pos, void *key, unsigned int keySize) {
	LOG_INFO("HashMap: _Contains\n");
	
	LListElement *iter;
	for (iter = hashMap->arrayOfLists[pos].head; iter != NULL; iter = iter->next) {
		if (memcmp(iter->key, key, keySize) == 0) {
			return FOUND;
		}
	}
	
	return NOT_FOUND;
}

int _remove(HashMap *hashMap, unsigned int pos, void *key, unsigned int keySize) {
	LOG_INFO("HashMap: _Remove\n");
	
	LListElement *iter;
	LListElement *prev = NULL;
	int found = 0;
	for (iter = hashMap->arrayOfLists[pos].head; iter != NULL; prev = iter, iter = iter->next) {
		if (memcmp(iter->key, key, keySize) == 0) {
			if (prev != NULL) {
				prev->next = iter->next;
			} else {
				hashMap->arrayOfLists[pos].head = hashMap->arrayOfLists[pos].head->next;
			}
			found = 1;
			break;
		}
	}
	
	if (found) {
		free(iter->key);
		hashMap->_free(iter->data);
		free(iter);
		hashMap->arrayOfLists[pos].size--;
		return OK;
	}
	
	return HASHMAP_ERROR;
}

int init_hashmap(HashMap *hashMap, unsigned int totalLists, unsigned int maxListSize, void (*_f)(void *)) {
	LOG_INFO("HashMap: Init Hashmap\n");
	
	if (hashMap == NULL || totalLists == 0 || maxListSize == 0) {
		LOG_ERROR("HashMap: HASHMAP_ERROR\n");
		return HASHMAP_ERROR;
	}
	
	hashMap->arrayOfLists = (LinkedList *) calloc(totalLists, sizeof(LinkedList));
	if (hashMap->arrayOfLists == NULL) {
		LOG_ERROR("HashMap: NO_MEMORY_ERROR\n");
		return NO_MEMORY_ERROR;
	}
	
	hashMap->maxListSize = maxListSize;
	hashMap->totalLists = totalLists;
	hashMap->size = 0;
	hashMap->_free = _f;
	
	return OK;
}

int put_hashmap(HashMap *hashMap, void *key, unsigned int keySize, void *data, unsigned int dataSize) {
	LOG_INFO("HashMap: Put Hashmap\n");
	
	if (hashMap == NULL || key == NULL || keySize == 0 || data == NULL || dataSize == 0) {
		LOG_ERROR("HashMap: HASHMAP_ERROR\n");
		return HASHMAP_ERROR;
	}
	
	unsigned int pos = hash(key, keySize) % hashMap->totalLists;
	return _insert(hashMap, pos, key, keySize, data, dataSize);
}

void* get_ref_key_hashmap(HashMap *hashMap, void *key, unsigned int keySize) {
	LOG_INFO("HashMap: Get Ref Key HashMap\n");
	
	if (hashMap == NULL || key == NULL || keySize == 0) {
		LOG_ERROR("HashMap: HASHMAP_ERROR\n");
		return NULL;
	}
	
	unsigned int pos = hash(key, keySize) % hashMap->totalLists;
	return _get_ref_key(hashMap, pos, key, keySize);
}

void* get_hashmap(HashMap *hashMap, void *key, unsigned int keySize) {
	LOG_INFO("HashMap: Get Hashmap\n");
	
	if (hashMap == NULL || key == NULL || keySize == 0) {
		LOG_ERROR("HashMap: HASHMAP_ERROR\n");
		return NULL;
	}
	
	if (hashMap->size == 0) {
		return NULL;
	}
	
	unsigned int pos = hash(key, keySize) % hashMap->totalLists;
	return _get(hashMap, pos, key, keySize);
}

int remove_hashmap(HashMap *hashMap, void *key, unsigned int keySize) {
	LOG_INFO("HashMap: Remove Hashmap\n");
	
	if (hashMap == NULL || key == NULL || keySize == 0) {
		LOG_ERROR("HashMap: HASHMAP_ERROR\n");
		return HASHMAP_ERROR;
	}
	
	unsigned int pos = hash(key, keySize) % hashMap->totalLists;
	return _remove(hashMap, pos, key, keySize);
}

int find_and_remove_hashmap(HashMap *hashMap, void *data, unsigned int dataSize) {
	LOG_INFO("HashMap: Find and Remove HashMap\n");
	
	if (hashMap == NULL || data == NULL || dataSize == 0) {
		LOG_ERROR("HashMap: HASHMAP_ERROR\n");
		return HASHMAP_ERROR;
	}
	
	int i;
	int found = 0;
	for (i = 0; i < hashMap->totalLists; i++) {
		LListElement *iter;
		for (iter = hashMap->arrayOfLists[i].head; iter != NULL; iter = iter->next) {
			if (memcmp(iter->data, data, dataSize) == 0 && iter->dataSize == dataSize) {
				found = 1;
				_remove(hashMap, i, iter->key, iter->keySize);				
				break;
			}
		}
		if (found) {
			break;
		}
	}
	
	if (found) {
		return OK;
	} else {
		return HASHMAP_ERROR;
	}
	
}

int contains_hashmap(HashMap *hashMap, void *key, unsigned int keySize) {
	LOG_INFO("HashMap: Contains Hashmap\n");
	
	if (hashMap == NULL || key == NULL || keySize == 0) {
		LOG_ERROR("HashMap: HASHMAP_ERROR\n");
		return HASHMAP_ERROR;
	}
	
	unsigned int pos = hash(key, keySize) % hashMap->totalLists;
	return _contains(hashMap, pos, key, keySize);
}

void free_hashmap(HashMap *hashMap) {
	unsigned int i;
	LListElement *iter;
	LListElement *temp;
	for (i = 0; i < hashMap->totalLists; i++) {
		for (iter = hashMap->arrayOfLists[i].head; iter != NULL;) {
			temp = iter;
			iter = iter->next;
			free(temp->key);
			if (hashMap->_free != free) {
				hashMap->_free(temp->data);
			}
			free(temp->data);
			free(temp);
		}
	}
	free(hashMap->arrayOfLists);
	hashMap->size = 0;
	hashMap->arrayOfLists = NULL;
	hashMap->totalLists = 0;
}
/*
int main() {
	HashMap map;
	int k1 = 10;
	int k2 = 30;
	int k3 = 50;
	int k4 = 100;
	
	char *d1 = "10";
	char *d2 = "30";
	char *d3 = "50";
	char *d4 = "100";
	
	init_hashmap(&map, 2, 1);
	
	put_hashmap(&map, &k1, sizeof(int), d1, strlen(d1));
	put_hashmap(&map, &k2, sizeof(int), d2, strlen(d2));
	put_hashmap(&map, &k3, sizeof(int), d3, strlen(d3));
	put_hashmap(&map, &k4, sizeof(int), d4, strlen(d4));
	
	print(&map);
	
	remove_hashmap(&map, &k1, sizeof(int));
	print(&map);
	
	printf("Contains 10 : %s\n", contains_hashmap(&map, &k1, sizeof(int)) == 1 ? "TRUE" : "FALSE");
	printf("Contains 50 : %s\n", contains_hashmap(&map, &k3, sizeof(int)) == 1 ? "TRUE" : "FALSE");
	
	free_hashmap(&map);
}*/
