#ifndef _FTPUTILITIES_H
#define _FTPUTILITIES_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include "parseUtilities.h"

struct hostent* getHostInfo(char* hostname);

int getResponse(int sockfd, char* response);
int communicate(int sockfd, char* com);

int openDataPort(char* host, int receivedPort, char* filename);
int receiveFile(int sockfd, char* filename);
#endif