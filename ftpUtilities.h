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

pid_t child;

void handler();

struct hostent* getHostInfo(char* hostname);

int openTCPandConnectServer(char* hostname, unsigned int port);
int openDataPort(char* hostname, int receivedPort);
int downloadFile(int dataSocketFD, char* filename);

int readLine(int sockfd, char* line);
int getResponse(int sockfd, char* response, int cleanBuffer);
int communicate(int sockfd, char* com);

int receiveFile(int sockfd, char* filename);

int checkIfServerReady(int commandSocketFD);
int setUsername(int commandSocketFD, char* username);
int setPassword(int commandSocketFD, char* password);
int setPassiveMode(int commandSocketFD);
int retrieveFile(int commandSocketFD, char* fileUrl);

int cleanUpSocket(int socketfd);
#endif