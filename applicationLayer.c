#include "applicationLayer.h"

int sendFile(char* file, size_t fileSize, char* fileName) {
	char* startPacket; 
	size_t startPacketSize = createControlPacket(startPacket, CONTROL_START, fileSize, fileName);
	char* endPacket;
	size_t endPacketSize  = createControlPacket(endPacket, CONTROL_END, fileSize, fileName);

	//Calculate the number of packets we will need based on the defined maximum packet size
	unsigned int numberOfPackets = fileSize / linkLayerConf.maxPacketSize;
	//Checks the remainder of previous division to check if we need an extra packet or not
	if((fileSize % linkLayerConf.maxPacketSize) > 0)
		numberOfPackets++;

	if(sendPacket(startPacket, startPacketSize) == -1)
		return -1;
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
	if(sendPacket(endPacket, endPacketSize) == -1)
		return -1;

	return 0;
}

char* createDataPacket(unsigned int sequenceNumber, size_t dataFieldLength, char* data);

char* createControlPacket(size_t* sizeOfPacket, char controlField, size_t fileSize, char* fileName);