#include "server.h"
#include "consumer.h"
#include "cache.h"
#include "types.h"

#include <sys/select.h>

typedef struct _ServerInfo {
	int sockFD;
	struct sockaddr_in servAddr;
}ServerInfo;

ServerInfo servInfo;
Cache cache;

int setupServer(int port) {
	
	LOG_INFO("Server: Setup Server, Listening port => %d\n", port);
	
	servInfo.sockFD = socket(SOCKET_DOMAIN, SOCKET_TYPE, SOCKET_PROTOCOL);
	if (servInfo.sockFD < 1) {
		LOG_ERROR("Server: SOCKET_ERROR\n");
		return SOCKET_ERROR;
	}
	
	int optval = 1;
	setsockopt(servInfo.sockFD, SOL_SOCKET, SO_REUSEADDR, (uint8_t *)&optval, sizeof(optval));
	
	memset(&servInfo.servAddr, 0, sizeof(struct sockaddr_in));
	servInfo.servAddr.sin_family = SOCKET_DOMAIN;
	servInfo.servAddr.sin_port = htons(port);
	servInfo.servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (bind(servInfo.sockFD, (struct sockaddr *)&servInfo.servAddr, sizeof(servInfo.servAddr)) < 0) {
		LOG_ERROR("Server: BIND ERROR\n");
		return BIND_ERROR;
	}
	
	if (listen(servInfo.sockFD, MAX_LISTEN) < 0) {
		LOG_ERROR("Server: LISTEN_ERROR\n");
		return LISTEN_ERROR;
	}
	
	LOG_INFO("Server: Setup Cache\n");	
	return init_cache(&cache, 10485760);
	
	return OK;
}

void clearServer() {
	clear_client();
	clear_consumer();
}

int runServer() {	
	int maxDes = servInfo.sockFD;
	fd_set fdSet;
	
	LOG_INFO("Server: Run Server\n");
	
	if (setupConsumer(&cache) != OK) {
		LOG_ERROR("Server: CONSUMER_ERROR\n");
		return CONSUMER_ERROR;
	}
	
	if (setupClient(21) != OK) {
		LOG_ERROR("Server: CLIENT_ERROR\n");
		clear_consumer();
		return CLIENT_ERROR;
	}
	
	while (1) {
		FD_ZERO(&fdSet);
		
		FD_SET(servInfo.sockFD, &fdSet);
		
		if (select(maxDes + 1, &fdSet, NULL, NULL, NULL) < 0) {
			LOG_ERROR("Server: SELECT_ERROR\n");
			continue;
		}
		
		LOG_INFO("Server: Connection received\n");
		
		if (FD_ISSET(servInfo.sockFD, &fdSet)) {
			socklen_t addrLen = sizeof(struct sockaddr_in);
			struct sockaddr_in clAddr;
			int clSock;
			
			if ( (clSock = accept(servInfo.sockFD, (struct sockaddr *) &clAddr, &addrLen)) < 0 ) {
				LOG_ERROR("Server: ACCEPT_ERROR\n");
			} else {
				if (insertSock_clients(clSock) != OK) {
					LOG_ERROR("Server: CONSUMER_ERROR\n");
					sendHttpBadReqMsg(clSock);
				}	
			}
		}
	}
	
	return OK;
}
