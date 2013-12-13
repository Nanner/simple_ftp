#ifndef _FTPUTILITIES_H
#define _FTPUTILITIES_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include "parseUtilities.h"

struct hostent* getHostInfo(char* hostname);

int openTCPandConnectServer(char* hostname, unsigned int port);
int openDataPort(char* hostname, int receivedPort);

int readLine(int sockfd, char* line);
int getResponse(int sockfd, char* response, int verbose);
int communicate(int sockfd, char* com, int verbose);

int checkIfServerReady(int commandSocketFD, int verbose);
int setUsername(int commandSocketFD, char* username, int verbose);
int setPassword(int commandSocketFD, char* password, int verbose);
int setPassiveMode(int commandSocketFD, int verbose);
int retrieveFile(int commandSocketFD, char* fileUrl, int verbose);

int downloadFile(int dataSocketFD, char* filename, int fileSize);
int receiveFile(int sockfd, char* filename, int fileSize);

#endif