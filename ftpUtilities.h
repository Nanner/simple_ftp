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

int openTCPandConnectServer(char* hostname, unsigned int port);
int openDataPortAndDownloadFile(char* hostname, int receivedPort, char* filename);

int getResponse(int sockfd, char* response);
int communicate(int sockfd, char* com);

int receiveFile(int sockfd, char* filename);

int checkIfServerReady(int commandSocketFD);
int setUsername(int commandSocketFD, char* username);
int setPassword(int commandSocketFD, char* password);
int setPassiveMode(int commandSocketFD);
int retrieveFile(int commandSocketFD, char* fileUrl);
#endif