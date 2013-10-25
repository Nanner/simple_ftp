#ifndef _FRAME_H
#define _FRAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linkLayerProtocol.h"

#define TRANSMITTER 1
#define RECEIVER 0

#define MAX_INFO_LENGTH 256

//Frame address values
#define RECEIVER_ADDRESS 0x03
#define SENDER_ADDRESS 0x01

//Frame control values
//U/S frames
#define SET 0x03
#define DISC 0x0B
#define UA 0x07
#define RR_0 0x05
#define RR_1 0x25
#define REJ_0 0x01
#define REJ_1 0x21
//I frames
#define INFO_0 0x00
#define INFO_1 0x02

//Frame header and trailer value
#define FRAMEFLAG 0x7E

//Indexes for a char* frame
#define BASE_FRAME_SIZE 6
#define FHEADERFLAG 0
#define FADDRESS 1
#define FCONTROL 2
#define FBCC1 3
#define FDATA 4
#define FBCC2(dataSize) 4 + dataSize
#define FTRAILERFLAG(dataSize) 5 + dataSize

//Error codes
#define FRAME_HEADER_ERROR -1
#define FRAME_INFO_ERROR -2

unsigned char* createSupervisionFrame(unsigned char address, unsigned char control, size_t maxInformationSize);
unsigned char* createInfoFrame(unsigned char address, unsigned char control, unsigned char information[], size_t infoLength, size_t maxInformationSize);

int copyInfo(unsigned char* destinationFrame, unsigned char source[], size_t length, size_t maxInformationSize);
void getInfo(unsigned char* frame, unsigned char destination[], size_t length);

unsigned char createBCC1(unsigned char address, unsigned char control);
unsigned char createBCC2(unsigned char information[], size_t maxInformationSize);
int validBCC1(unsigned char* frame);
int validBCC2(unsigned char* frame, size_t maxInformationSize);
int checkForErrors(unsigned char* frame, size_t maxInformationSize, int role);

void flipbit(unsigned char* byte, unsigned bitNumber);
#endif
