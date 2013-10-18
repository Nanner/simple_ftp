#ifndef _SUPERVISIONFRAME_H
#define _SUPERVISIONFRAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INFO_LENGTH 256

#define RECEIVER_ADDRESS 0x03
#define SENDER_ADDRESS 0x01

#define SET 0x03
#define DISC 0x0B
#define UA 0x07

#define FRAMEFLAG 0x7E

#define BASE_FRAME_SIZE 6
#define FHEADER 0
#define FADDRESS 1
#define FCONTROL 2
#define FBCC1 3
#define FDATA 4
#define FBCC2(dataSize) 4 + dataSize
#define FTRAILER(dataSize) 5 + dataSize

char* createSupervisionFrame(char address, char control, size_t maxInformationSize);
char* createInfoFrame(char address, char control, char information[], size_t infoLength, size_t maxInformationSize);

int copyInfo(char* destinationFrame, char source[], size_t length, size_t maxInformationSize);
void getInfo(char* frame, char destination[], size_t length);

char createBCC1(char address, char control);
char createBCC2(char information[], size_t maxInformationSize);
int validBCC1(char* frame);
int validBCC2(char* frame, size_t maxInformationSize);

#endif
