#ifndef _APPLICATION_LAYER_H
#define _APPLICATION_LAYER_H

#include <sys/types.h>

#include "linkLayerProtocol.h"

#define CONTROL_DATA 0
#define CONTROL_START 1
#define CONTROL_END 2

#define BASE_DATA_PACKET_SIZE 4

typedef struct {
       int fileDescriptor;
       int status;
} ApplicationLayer;

extern ApplicationLayer applicationLayerConf;

int sendFile(char* file, size_t fileSize, char* fileName);

char* createDataPacket(unsigned int sequenceNumber, size_t dataFieldLength, char* data);

char* createControlPacket(size_t* sizeOfPacket, char controlField, size_t fileSize, char* fileName);

#endif