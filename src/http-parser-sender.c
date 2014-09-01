#include "http-parser-sender.h"
#include "types.h"

int findHttpMessageType(char *httpMsg, size_t len) {
	LOG_INFO("HttpParserSender: Find Http Message Type\n");
	if (httpMsg == NULL || len == 0) {
		LOG_ERROR("HttpParserSender: HTTP_PARSER_ERROR\n");
		return HTTP_PARSER_ERROR;
	}
	
	if (strncmp(httpMsg, GET_STR, 3) == 0) {
		return HTTP_GET;
	} else if (strncmp(httpMsg, HTTP_STR, 4) == 0) {
		return HTTP_RESPONSE;
	} else {
		return HTTP_PARSER_ERROR;
	}
}

int parseHttpGetMsg(char *httpMsg, size_t len, HttpGet *httpGet) {
	LOG_INFO("HttpParserSender: Parse Http Get Message\n");
	LOG_INFO("<HTTP_MSG>%s</HTTP_MSG>\n", httpMsg);
	
	int hostLen, reqLen;
	char *pReqLIndex;
	char *p, *save, *delim = "\r\n";
	int count = 0;
	for ( (p = strtok_r(httpMsg, delim, &save)); p; p = strtok_r(NULL, delim, &save)) {
		count++;
		
		if (strncmp(p, "GET ", 4) == 0) {
			pReqLIndex = strrchr(p + 4, ' ');
			reqLen = (pReqLIndex - p) - 4 + 1; 
			
			httpGet->request = (char *) malloc(sizeof(char) * reqLen);
			if (httpGet->request == NULL) {
				LOG_ERROR("HttpParserSender: NO_MEMORY_ERROR 0\n");
				return NO_MEMORY_ERROR;
			}
			
			strncpy(httpGet->request, p + 4, reqLen - 1);
			httpGet->request[reqLen - 1] = '\0';
			
		} else if (strncmp(p, "Host: ", 6) == 0) {
			hostLen = strlen(p) - 6 + 1;
			
			httpGet->host = (char *) malloc(sizeof(char) * hostLen);
			if (httpGet->host == NULL) {
				free(httpGet->request);
				LOG_ERROR("HttpParserSender: NO_MEMORY_ERROR 1\n");
			}
			
			strncpy(httpGet->host, p + 6, hostLen - 1);
			httpGet->host[hostLen - 1] = '\0';
		}
	}
	
	LOG_INFO("HttpParserSender: Request => %s\n", httpGet->request);
	LOG_INFO("HttpParserSender: Host => %s\n", httpGet->host);
	
	httpGet->reqLen = reqLen;
	httpGet->msg = httpMsg;
	httpGet->len = len;
	
	return OK;
}


void sendHttpBadReqMsg(int sock) {
	char *httpMsg = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";
	size_t len = strlen(httpMsg);
	
	send(sock, httpMsg, len, 0);
	
	close(sock);
}

void sendHttpNotFoundMsg(int sock) {
	char *httpMsg = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";
	size_t len = strlen(httpMsg);
	
	send(sock, httpMsg, len, 0);
	
	close(sock);
}

int sendHttpGetMsgToServer(HttpGet *httpGet) {
	LOG_INFO("HttpParserSender: Send Http Get Message To Server\n");
	struct sockaddr_in servAddr;
	int servSock;
	ssize_t bytesSent;
	struct addrinfo hints, *res;
	
	servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (servSock < 0) {
		LOG_ERROR("HttpParserSender: SOCKET_ERROR\n");
		return SOCKET_ERROR;
	}
	
	memset(&servAddr, 0, sizeof(struct sockaddr_in));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(80);
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
	
	if ( getaddrinfo(httpGet->host, NULL, &hints, &res) != 0 ) {
		LOG_ERROR("HttpParserSender: GETADDRINFO_ERROR\n");
		return GETADDRINFO_ERROR;
	}
	
	servAddr.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	
	if (connect(servSock, (struct sockaddr *) &servAddr, sizeof(struct sockaddr_in)) < 0) {
		LOG_ERROR("HttpParserSender: CONNECT_ERROR\n");
		return CONNECT_ERROR;
	}
	
	bytesSent = send(servSock, httpGet->msg, httpGet->len, 0);
	if (bytesSent != httpGet->len) {
		LOG_ERROR("HttpParserSender: SEND_ERROR\n");
		close(servSock);
		return SEND_ERROR;
	}
	
	free(res);
	
	return servSock;
}

int sendSingleHttpResponseMsg(char *buffer, ssize_t len, int sock) {
	LOG_INFO("HttpParserSender: Send_Single_Http_Response_Message\n");
	
	if (buffer == NULL || len == 0) {
		LOG_ERROR("HttpParserSender: SEND_ERROR\n");
		return SEND_ERROR;
	}
	
	ssize_t sentBytes = send(sock, buffer, len, 0);
	if (sentBytes != len) {
		LOG_ERROR("HttpParserSender: SEND_ERROR\n");
		return SEND_ERROR;
	}
	
	close(sock);
	
	return OK;
}

int sendHttpResponseMsg(char *buffer, ssize_t len, Queue *socks) {
	LOG_INFO("HttpParserSender: Send_Http_Response_Message\n");
	
	if (buffer == NULL || len == 0) {
		LOG_ERROR("HttpParserSender: SEND_ERROR\n");
		return SEND_ERROR;
	}
	
	QElement *iter;
	for (iter = socks->head; iter != NULL; iter = iter->next) {
		LOG_INFO("\tHttpParserSender: Sending response to %d\n", iter->clSock);
		ssize_t sentBytes = send(iter->clSock, buffer, len, 0);
		if (sentBytes != len) {
			LOG_ERROR("HttpParserSender: SEND_ERROR\n");
			return SEND_ERROR;
		} 
		close(iter->clSock);
	}
	
	return OK;
}

void sendRequestTimeout(Queue *socks) {
	QElement *iter;
	char *httpMsg = "HTTP/1.1 408 Request Timeout\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";
	size_t len = strlen(httpMsg);
	
	for (iter = socks->head; iter != NULL; iter = iter->next) {
		send(iter->clSock, httpMsg, len, 0);
		close(iter->clSock);
	}
}

void clear_httpGetMsg(HttpGet *msg) {
	msg->len = 0;
	msg->reqLen = 0;
	free(msg->request);
	free(msg->host);
}

/*
int main() {
	char *get = "GET /index23483`17671.html HTTP/1.1\r\nHost: www.example2123124.com\r\n\r\n";
	HttpGet httpGet;
	if (parseHttp(get, strlen(get), &httpGet) != OK) {
		fprintf(stderr, "Error\n");
	}
	
	return 0;
}*/
