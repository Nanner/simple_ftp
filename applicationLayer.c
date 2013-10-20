#include "applicationLayer.h"

int sendFile(char* file, size_t fileSize, char* fileName) {
	if(fileSize > MAX_FILESIZE_ALLOWED) {
		printf("This file is too large to be sent!\n");
		return -1;
	}

	if(strlen(fileName) > (applicationLayerConf.maxPacketSize - (BASE_DATA_PACKET_SIZE + sizeof(size_t)) ) ) {
		printf("The filename is too large!\n");
		return -1;
	}
	size_t startPacketSize;
	char* startPacket = createControlPacket(&startPacketSize, CONTROL_START, fileSize, fileName);
	size_t endPacketSize;
	char* endPacket = createControlPacket(&endPacketSize, CONTROL_END, fileSize, fileName);

	//Calculate the number of packets we will need based on the defined maximum packet size
	unsigned int numberOfPackets = fileSize / applicationLayerConf.maxDataFieldSize;
	//Checks the remainder of previous division to check if we need an extra packet or not
	if((fileSize % applicationLayerConf.maxDataFieldSize) > 0)
		numberOfPackets++;

	if(sendPacket(startPacket, startPacketSize) == -1) {
		free(startPacket);
		return -1;
	}
	free(startPacket);
	unsigned int currentPacket = 0;
	for(; currentPacket < numberOfPackets; currentPacket++) {
		unsigned int dataFieldSize;
		if(currentPacket == numberOfPackets - 1) {
			dataFieldSize = fileSize - currentPacket * applicationLayerConf.maxDataFieldSize; //We do this to ensure we don't copy more than the needed bytes in the last packet
		}
		else
			dataFieldSize = applicationLayerConf.maxDataFieldSize;

		char dataField[applicationLayerConf.maxDataFieldSize];
		memcpy(dataField, &file[currentPacket * applicationLayerConf.maxDataFieldSize], dataFieldSize);
		char* dataPacket = createDataPacket(currentPacket, dataFieldSize, dataField);
		if(sendPacket(dataPacket, dataFieldSize + BASE_DATA_PACKET_SIZE) == -1) {
			free(dataPacket);
			return -1;
		}
		free(dataPacket);
	}
	if(sendPacket(endPacket, endPacketSize) == -1) {
		free(endPacket);
		return -1;
	}
	free(endPacket);

	return 0;
}

char* receiveFile(size_t* fileSize, char* fileName) {
	printf("Waiting to receive file...\n");
	char fileNameReceived[applicationLayerConf.maxPacketSize - (BASE_DATA_PACKET_SIZE + sizeof(size_t))];
	//char* fileNameReceived;
	size_t fileSizeReceived;
	size_t fileSizeLeft;
	unsigned int expectedSequence = 0;
	unsigned int fileIndexToCopyTo = 0;
	char* file;

	char* startPacket = malloc(applicationLayerConf.maxPacketSize);
	if(receivePacket(startPacket, applicationLayerConf.maxPacketSize) != -1) {
		if(startPacket[CONTROL_INDEX] == CONTROL_START) {
			printf("Starting file reception.\n");
			unsigned int fileSizeSize = startPacket[FILESIZE_SIZE_INDEX];
			unsigned int fileNameSize = startPacket[FILENAME_SIZE_INDEX(fileSizeSize)];
			//fileNameReceived = malloc(fileNameSize);
			memcpy(&fileSizeReceived, &startPacket[FILESIZE_INDEX], fileSizeSize);
			memcpy(fileNameReceived, &startPacket[FILENAME_INDEX(fileSizeSize)], fileNameSize);

			printf("\nReceiving %s, Expected size: %lu\n\n", fileNameReceived, fileSizeReceived);
			fileSizeLeft = fileSizeReceived;
			file = malloc(fileSizeReceived);
		}
	}
	else
		return NULL;

	char* packet = malloc(applicationLayerConf.maxPacketSize);
	unsigned int transmissionOver = 0;
	while(!transmissionOver) {
		if(receivePacket(packet, applicationLayerConf.maxPacketSize) != -1) {
			if(packet[CONTROL_INDEX] == CONTROL_END)
				transmissionOver = 1;
			//TODO sequence number must be mod 255!!!
			else if(packet[CONTROL_INDEX] == CONTROL_DATA && packet[SEQUENCE_INDEX] == expectedSequence) {
				unsigned int l1 = packet[L1_INDEX];
				unsigned int l2 = packet[L2_INDEX];
				printf("Received l2: %u, l1: %u\n", l2, l1);
				size_t dataSize = 256 * l2 + l1;

				memcpy(&file[fileIndexToCopyTo], &packet[DATA_INDEX], dataSize);
				fileIndexToCopyTo += dataSize;
				fileSizeLeft -= dataSize;
				printf("\n%lu bytes left...\n\n", fileSizeLeft);
				expectedSequence++;
			}
		}
		else return NULL;
	}
	if(fileSizeLeft == 0 && compareControlPackets(startPacket, packet) == 0)
		return file;
	else
		return NULL;
}

char* createDataPacket(unsigned int sequenceNumber, size_t dataFieldLength, char* data) {
	unsigned int l1, l2;
	l2 = dataFieldLength / 256;
	l1 = dataFieldLength % 256;
	printf("\nDataSize: %lu, L2: %u, L1: %u\n\n",dataFieldLength, l2, l1);

	char* dataPacket = malloc(BASE_DATA_PACKET_SIZE + dataFieldLength);
	dataPacket[CONTROL_INDEX] = CONTROL_DATA;
	dataPacket[SEQUENCE_INDEX] = sequenceNumber;
	dataPacket[L2_INDEX] = l2;
	dataPacket[L1_INDEX] = l1;
	memcpy(&dataPacket[DATA_INDEX], data, dataFieldLength);

	return dataPacket;
}

char* createControlPacket(size_t* sizeOfPacket, char controlField, size_t fileSize, char* fileName) {
	*sizeOfPacket = 1 + 2 + sizeof(size_t) + 2 + strlen(fileName);

	char* controlPacket = malloc(*sizeOfPacket);

	controlPacket[CONTROL_INDEX] = controlField;

	controlPacket[1] = CONTROL_TYPE_FILESIZE;
	controlPacket[2] = sizeof(size_t);
	memcpy(&controlPacket[3], &fileSize, sizeof(size_t));
	unsigned int nextIndex = 3 + sizeof(size_t);
	controlPacket[nextIndex] = CONTROL_TYPE_FILENAME;
	nextIndex++;
	controlPacket[nextIndex] = strlen(fileName);
	nextIndex++;
	memcpy(&controlPacket[nextIndex], fileName, strlen(fileName));

	return controlPacket;
}

int compareControlPackets(char* packet1, char* packet2) {
	size_t packet1FileSize, packet2FileSize;
	char* packet1FileName;
	char* packet2FileName;
	unsigned int sizeOfFileSizeParameter1 = packet1[FILESIZE_SIZE_INDEX];
	unsigned int sizeOfFileSizeParameter2 = packet2[FILESIZE_SIZE_INDEX];

	//If the first parameter type is the same and have the same length in both packets
	if(packet1[1] == packet2[1] && sizeOfFileSizeParameter1 == sizeOfFileSizeParameter2) {
		//And if it is a filesize parameter type as expected
		if(packet1[1] == CONTROL_TYPE_FILESIZE) { 
			memcpy(&packet1FileSize, &packet1[FILESIZE_INDEX], sizeOfFileSizeParameter1);
			memcpy(&packet2FileSize, &packet2[FILESIZE_INDEX], sizeOfFileSizeParameter2);

			//Check if the filesizes are the same
			if(packet1FileSize != packet2FileSize)
				return -1;
		}
		else
			return -2; //Something happened, the parameters aren't in the expected order
	}
	else
		return -2;

	//If the second parameter type is the same and have the same length in both packets
	if(packet1[3 + sizeOfFileSizeParameter1] == packet2[3 + sizeOfFileSizeParameter2] && packet1[FILENAME_SIZE_INDEX(sizeOfFileSizeParameter1)] == packet2[FILENAME_SIZE_INDEX(sizeOfFileSizeParameter2)]) {
		//And if it is a filename parameter type as expected
		if(packet1[3 + sizeOfFileSizeParameter1] == CONTROL_TYPE_FILENAME) {
			size_t fileNameSize1 = packet1[FILENAME_SIZE_INDEX(sizeOfFileSizeParameter1)];
			size_t fileNameSize2 = packet2[FILENAME_SIZE_INDEX(sizeOfFileSizeParameter2)];
			packet1FileName = malloc(fileNameSize1); 
			packet2FileName = malloc(fileNameSize2);
			memcpy(packet1FileName, &packet1[FILENAME_INDEX(sizeOfFileSizeParameter1)], fileNameSize1);
			memcpy(packet2FileName, &packet2[FILENAME_INDEX(sizeOfFileSizeParameter2)], fileNameSize2);

			//Check if the filenames are the same
			if(strcmp(packet1FileName, packet2FileName) != 0) {
				free(packet1FileName);
				free(packet2FileName);
				return -1;
			}

			free(packet1FileName);
			free(packet2FileName);
		}
		else
			return -2; //Something happened, the parameters aren't in the expected order
	}
	else
		return -2;

	return 0;
}

char* readFile(char *fileName, size_t* fileSize) {
	FILE* file;
	char* buffer;

	file = fopen(fileName, "rb");
	if (!file) {
		fprintf(stderr, "Unable to open file %s, does it exist?\n", fileName);
		return NULL;
	}
	
	fseek(file, 0, SEEK_END);
	*fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	buffer = (char *) malloc(*fileSize+1);
	if (!buffer) {
		fprintf(stderr, "Memory error!");
        fclose(file);
		return NULL;
	}

	fread(buffer, *fileSize, 1, file);
	fclose(file);

	return buffer;
}