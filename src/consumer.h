#ifndef _CONSUMER_H_
#define _CONSUMER_H_

int setupConsumer();

int insertSock_clients(int);

int insertSock_webservers(int);

void sendRequestTimeoutMsgToClients(int);

void clear_consumer();

#endif
