#include "server.h"
#include "types.h"

#include <signal.h>

void sigintHandler(int sig) {
	clearServer();
	exit(0);
}

void usage(char *progName) {
	fprintf(stderr, "Usage: %s <-p Listening Port>\n", progName);
	exit(1);
}

int main(int argc, char *argv[]) {
	int port;
	int c;
	char *strPort;
	
	if (argc != 3) {
		usage(argv[0]);
	}
	
	while ( (c = getopt(argc, argv, "p:")) != -1) {
		switch (c) {
			case 'p':
				strPort = optarg;
				break;
			case '?':
			default:
				usage(argv[0]);
		}
	}
	
	signal(SIGINT, sigintHandler);
	
	port = atoi(strPort);
	
	setupServer(port);
	
	runServer();
}
