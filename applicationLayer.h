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
#define CONTROL_TYPE_FILENAME 1

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