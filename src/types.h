#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include <pthread.h>

#include "log.h"
#include "error.h"

#define SOCKET_DOMAIN AF_INET
#define SOCKET_TYPE SOCK_STREAM
#define SOCKET_PROTOCOL IPPROTO_TCP

#define MAX_LISTEN 1024
#define THREAD_POOL_SIZE 20
#define BUFSIZE 65536

#define HTTP_GET 2
#define HTTP_RESPONSE 3

#define GET_STR "GET "
#define HOST_STR "Host: "
#define CRNL_STR "\r\n"
#define HTTP_STR "HTTP"

#endif
