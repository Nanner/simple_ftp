#ifndef _APPLICATION_LAYER_H
#define _APPLICATION_LAYER_H

#include <sys/types.h>

#include "linkLayerProtocol.h"

#define CONTROL_INDEX 0

#define SEQUENCE_INDEX 1
#define L2_INDEX 2
#define L1_INDEX 3
#define DATA_INDEX 4

#define CONTROL_TYPE_FILESIZE 0
#define FILESIZE_SIZE_INDEX 2
#define FILESIZE_INDEX 3

#define CONTROL_TYPE_FILENAME 1
#define FILENAME_SIZE_INDEX(fileSizeFieldSize) 4 + fileSizeFieldSize
#define FILENAME_INDEX(fileSizeFieldSize) 5 + fileSizeFieldSize

#define CONTROL_DATA 0
#define CONTROL_START 1
#define CONTROL_END 2

#define BASE_DATA_PACKET_SIZE 4

#define MAX_FILESIZE_ALLOWED 1073741824

typedef struct {
       int fileDescriptor;
       int status;
       size_t maxPacketSize;
       size_t maxDataFieldSize;
} ApplicationLayer;

extern ApplicationLayer applicationLayerConf;

int sendFile(char* file, size_t fileSize, char* fileName);

char* receiveFile(size_t* fileSize, char* fileName);

char* createDataPacket(unsigned char sequenceNumber, size_t dataFieldLength, char* data);

char* createControlPacket(size_t* sizeOfPacket, char controlField, size_t fileSize, char* fileName);

int compareControlPackets(char* packet1, char* packet2);

char* readFile(char *fileName, size_t* fileSize);

int writeFile(char* fileBuffer, char* fileName, size_t fileSize);

#endif