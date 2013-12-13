#ifndef _PARSEUTILITIES_H
#define _PARSEUTILITIES_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h> //dealing with file paths

#define MAX_SIZE 8192
#define MAX_LINE_SIZE 1024
#define INT_DIGITS 19

int hasLoginField(char* url);
int parseUsername(char* username, char* url);
int parsePassword(char* password, char* url);

int parseHostnameAndUrl(char* hostname, char* fileUrl, char* url);
char* parseFilename(char* fileUrl);

int getReturnCode(char* response);
void getLineIdentifier(char* response, char* ident);

int calculatePasvPort(int* port, char* response);

int getFileSize(char* response);

char* itoa(int i);

#endif