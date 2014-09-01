#ifndef _HTTP_PARSER_SENDER_H_
#define _HTTP_PARSER_SENDER_H_

#include "types.h"
#include "queue.h"

typedef struct _HttpGet {
	char *request;
	size_t reqLen;
	char *msg;
	ssize_t len;
	char *host;
} HttpGet;

int findHttpMessageType(char *, size_t);

int parseHttpGetMsg(char *, size_t, HttpGet *);

void sendHttpNotFoundMsg(int);

void sendHttpBadReqMsg(int);

void sendRequestTimeout(Queue *);

int sendHttpGetMsgToServer(HttpGet *);

int sendHttpResponseMsg(char *, ssize_t, Queue *);

int sendSingleHttpResponseMsg(char *, ssize_t, int);

void clear_httpGetMsg(HttpGet *);

#endif
