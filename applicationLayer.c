#include "applicationLayer.h"

int sendFile(char* file, size_t fileSize, char* fileName) {
	size_t startPacketSize;
	char* startPacket = createControlPacket(&startPacketSize, CONTROL_START, fileSize, fileName);
	size_t endPacketSize;
	char* endPacket = createControlPacket(&endPacketSize, CONTROL_END, fileSize, fileName);

	//Calculate the number of packets we will need based on the defined maximum packet size
	unsigned int numberOfPackets = fileSize / linkLayerConf.maxPacketSize;
	//Checks the remainder of previous division to check if we need an extra packet or not
	if((fileSize % linkLayerConf.maxPacketSize) > 0)
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
			dataFieldSize = fileSize - currentPacket * linkLayerConf.maxPacketSize; //We do this to ensure we don't copy more than the needed bytes in the last packet
		}
		else
			dataFieldSize = linkLayerConf.maxPacketSize;

		char dataField[linkLayerConf.maxPacketSize];
		memcpy(dataField, &file[currentPacket * linkLayerConf.maxPacketSize], dataFieldSize);
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

char* createDataPacket(unsigned int sequenceNumber, size_t dataFieldLength, char* data) {
	unsigned int l1, l2;
	l2 = dataFieldLength / 256;
	l1 = (dataFieldLength % 256) * 256;

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