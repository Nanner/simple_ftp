#include "applicationLayer.h"


static unsigned char* createDataPacket(unsigned char sequenceNumber, size_t dataFieldLength, unsigned char* data);
static unsigned char* createControlPacket(size_t* sizeOfPacket, unsigned char controlField, size_t fileSize, char* fileName);
static int compareControlPackets(unsigned char* packet1, unsigned char* packet2);

// loadBar code based on
// http://www.rosshemsley.co.uk/2011/02/creating-a-progress-bar-in-c-or-any-other-console-app/
// Process has done x out of n rounds,
// and we want a bar of width w and resolution r.
static inline void loadBar(int x, int n, int r, int w, int doneSize, int totalSize) {
    // Only update r times.
    if ( x % (n/r) != 0 ) return;
 
    // Calculuate the ratio of complete-to-incomplete.
    float ratio = x/(float)n;
    int   c     = ratio * w;
 
    // Show the percentage complete.
    printf("%3d%%: %d/%d B [", (int)(ratio*100), doneSize, totalSize );
 
    // Show the load bar.
    int y;
    for (y = 0; y < c; y++)
       printf("=");
 
    for (y = c; y < w; y++)
       printf(" ");
 
    // ANSI Control codes to go back to the
    // previous line and clear it.
   	printf("]\n");
	if(x < n)   
    	printf("\033[F\033[J");
}

int sendFile(unsigned char* file, size_t fileSize, char* fileName) {
	if(fileSize > MAX_FILESIZE_ALLOWED) {
		printf("This file is too large to be sent!\n");
		return -1;
	}

	if(strlen(fileName) > (applicationLayerConf.maxPacketSize - (BASE_DATA_PACKET_SIZE + sizeof(size_t)) ) || strlen(fileName) > 255) {
		printf("The filename is too large!\n");
		return -1;
	}
	size_t startPacketSize;
	unsigned char* startPacket = createControlPacket(&startPacketSize, CONTROL_START, fileSize, fileName);
	size_t endPacketSize;
	unsigned char* endPacket = createControlPacket(&endPacketSize, CONTROL_END, fileSize, fileName);

	unsigned int sizeDone = 0;

	//Calculate the number of packets we will need based on the defined maximum packet size
	unsigned long numberOfPackets = fileSize / applicationLayerConf.maxDataFieldSize;
	//Checks the remainder of previous division to check if we need an extra packet or not
	if((fileSize % applicationLayerConf.maxDataFieldSize) > 0)
		numberOfPackets++;

	if(sendData(startPacket, startPacketSize) == -1) {
		free(startPacket);
		return -1;
	}
	free(startPacket);
	unsigned long currentPacket = 0;
	unsigned long currentSequence = 0;
	loadBar(currentPacket, numberOfPackets, numberOfPackets, 50, sizeDone, fileSize);
	for(; currentPacket < numberOfPackets; currentPacket++) {
		unsigned long dataFieldSize;
		if(currentPacket == numberOfPackets - 1) {
            //We do this to ensure we don't copy more than the needed bytes in the last packet
			dataFieldSize = fileSize - currentPacket * applicationLayerConf.maxDataFieldSize;
		}
		else
			dataFieldSize = applicationLayerConf.maxDataFieldSize;

		sizeDone += dataFieldSize;

		unsigned char dataField[applicationLayerConf.maxDataFieldSize];
		memcpy(dataField, &file[currentPacket * applicationLayerConf.maxDataFieldSize], dataFieldSize);
		unsigned char* dataPacket = createDataPacket(currentPacket, dataFieldSize, dataField);
		if(sendData(dataPacket, dataFieldSize + BASE_DATA_PACKET_SIZE) == -1) {
			free(dataPacket);
			return -1;
		}
		free(dataPacket);

		if(currentSequence > 255)
			currentSequence = 0;
		else
			currentSequence++;

		loadBar(currentPacket+1, numberOfPackets, numberOfPackets, 50, sizeDone, fileSize);
	}
	if(sendData(endPacket, endPacketSize) == -1) {
		free(endPacket);
		return -1;
	}
	printf("\n\n");
	free(endPacket);

	return 0;
}

unsigned char* receiveFile(size_t* fileSize, char** fileName) {
	size_t fileSizeReceived;
	size_t fileSizeDone;
	size_t fileSizeLeft;
	unsigned int expectedSequence = 0;
	unsigned int fileIndexToCopyTo = 0;
	unsigned char* file;

	unsigned char* startPacket = malloc(applicationLayerConf.maxPacketSize);
	if(startPacket == NULL) {
		printf("Failed to allocate memory for startPacket, terminating\n");
		return NULL;
	}

	int receptionRes = receiveData(startPacket, applicationLayerConf.maxPacketSize);

	if(receptionRes == -2) {
		//We received a disconnect, end reception
		printf("Received disconnect order, terminating reception\n");
		free(startPacket);
		return NULL;
	}
	else if(receptionRes != -1) {
		if(startPacket[CONTROL_INDEX] == CONTROL_START) {

			unsigned char fileSizeSize = startPacket[FILESIZE_SIZE_INDEX];
			unsigned char fileNameSize = startPacket[FILENAME_SIZE_INDEX(fileSizeSize)];

			memcpy(&fileSizeReceived, &startPacket[FILESIZE_INDEX], fileSizeSize);

			char fileNameReceived[fileNameSize+1];
			memcpy(fileNameReceived, &startPacket[FILENAME_INDEX(fileSizeSize)], fileNameSize);
			fileNameReceived[fileNameSize] = '\0';

            *fileName = malloc(FILENAME_LEN);

			memcpy(*fileName, fileNameReceived, fileNameSize+1);

			*fileSize = fileSizeReceived;
			fileSizeLeft = fileSizeReceived;
			fileSizeDone = 0;
			file = malloc(fileSizeReceived);
			if(file == NULL) {
				printf("Failed to allocate memory for the file buffer, terminating\n");
				free(startPacket);
				return NULL;
			}
		}
	}
	else {
		printf("Failed to receive start packet data\n");
		free(startPacket);
		return NULL;
	}

	unsigned char* packet = malloc(applicationLayerConf.maxPacketSize);
	if(packet == NULL) {
		printf("Failed to allocate memory for packet, terminating\n");
		free(startPacket);
		return NULL;
	}
	unsigned int transmissionOver = 0;
	unsigned int barIndex = 0;

	//Calculate the number of packets we will need based on the defined maximum packet size
	unsigned long numberOfPackets = fileSizeReceived / applicationLayerConf.maxDataFieldSize;
	//Checks the remainder of previous division to check if we need an extra packet or not
	if((fileSizeReceived % applicationLayerConf.maxDataFieldSize) > 0)
		numberOfPackets++;

	loadBar(barIndex, (int)numberOfPackets, (int)numberOfPackets, 50, fileSizeDone, fileSizeReceived);
	barIndex++;

	while(!transmissionOver) {
		receptionRes = receiveData(packet, applicationLayerConf.maxPacketSize);
		if(receptionRes == -2) {
			//We received a disconnect, end reception
			printf("Received disconnect order, terminating reception\n");
			free(startPacket);
			free(file);
			free(packet);
			return NULL;
		}
		else if(receptionRes != -1) {
			if(packet[CONTROL_INDEX] == CONTROL_END) {
				transmissionOver = 1;
				printf("\n\n");
			}
			else if(packet[CONTROL_INDEX] == CONTROL_DATA && packet[SEQUENCE_INDEX] == expectedSequence) {
				unsigned char l1 = packet[L1_INDEX];
				unsigned char l2 = packet[L2_INDEX];

				size_t dataSize = 256 * l2 + l1;

				memcpy(&file[fileIndexToCopyTo], &packet[DATA_INDEX], dataSize);
				fileIndexToCopyTo += dataSize;
				fileSizeDone += dataSize;
				fileSizeLeft -= dataSize;
				loadBar(barIndex, (int)numberOfPackets, (int)numberOfPackets, 50, fileSizeDone, fileSizeReceived);
				barIndex++;
				expectedSequence++;
				if(expectedSequence > 255)
					expectedSequence = 0;
			}
		}
		else {
			printf("Failed to receive packet data\n");
			free(startPacket);
			free(file);
			free(packet);
			return NULL;
		}
	}
	if(fileSizeLeft == 0 && compareControlPackets(startPacket, packet) == 0) {
		free(startPacket);
		free(packet);
		return file;
	}
	else {
		if(fileSizeLeft != 0)
			printf("Didn't receive the expected byte number\n");
		if(compareControlPackets(startPacket, packet) != 0)
			printf("Start and end packets are different\n");

		free(startPacket);
		free(file);
		free(packet);

		return NULL;
	}
}

unsigned char* createDataPacket(unsigned char sequenceNumber, size_t dataFieldLength, unsigned char* data) {
	unsigned char l1, l2;
	l2 = dataFieldLength / 256;
	l1 = dataFieldLength % 256;

	unsigned char* dataPacket = malloc(BASE_DATA_PACKET_SIZE + dataFieldLength);
	if(dataPacket == NULL) {
		printf("Failed to allocate memory for data packet creation, terminating\n");
		return NULL;
	}
	dataPacket[CONTROL_INDEX] = CONTROL_DATA;
	dataPacket[SEQUENCE_INDEX] = sequenceNumber;
	dataPacket[L2_INDEX] = l2;
	dataPacket[L1_INDEX] = l1;
	memcpy(&dataPacket[DATA_INDEX], data, dataFieldLength);

	return dataPacket;
}

unsigned char* createControlPacket(size_t* sizeOfPacket, unsigned char controlField, size_t fileSize, char* fileName) {
	*sizeOfPacket = 1 + 2 + sizeof(size_t) + 2 + strlen(fileName);

	unsigned char* controlPacket = malloc(*sizeOfPacket);
	if(controlPacket == NULL) {
		printf("Failed to allocate memory for control packet creation, terminating\n");
		return NULL;
	}

	controlPacket[CONTROL_INDEX] = controlField;

	controlPacket[1] = CONTROL_TYPE_FILESIZE;
	controlPacket[2] = sizeof(size_t);
	memcpy(&controlPacket[3], &fileSize, sizeof(size_t));
	unsigned char nextIndex = 3 + sizeof(size_t);
	controlPacket[nextIndex] = CONTROL_TYPE_FILENAME;
	nextIndex++;
	controlPacket[nextIndex] = strlen(fileName);
	nextIndex++;
	memcpy(&controlPacket[nextIndex], fileName, strlen(fileName));

	return controlPacket;
}

int compareControlPackets(unsigned char* packet1, unsigned char* packet2) {
	size_t packet1FileSize, packet2FileSize;
	char* packet1FileName;
	char* packet2FileName;
	unsigned char sizeOfFileSizeParameter1 = packet1[FILESIZE_SIZE_INDEX];
	unsigned char sizeOfFileSizeParameter2 = packet2[FILESIZE_SIZE_INDEX];

	//If the first parameter type is the same and have the same length in both packets
	if(packet1[1] == packet2[1] && sizeOfFileSizeParameter1 == sizeOfFileSizeParameter2) {
		//And if it is a filesize parameter type as expected
		if(packet1[1] == CONTROL_TYPE_FILESIZE) { 
			memcpy(&packet1FileSize, &packet1[FILESIZE_INDEX], sizeOfFileSizeParameter1);
			memcpy(&packet2FileSize, &packet2[FILESIZE_INDEX], sizeOfFileSizeParameter2);

			//Check if the filesizes are the same
			if(packet1FileSize != packet2FileSize) {
				printf("Start and end packets file sizes are different\n");
				return -1;
			}
		}
		else {
			printf("Start and end parameters aren't in the expected order\n");
			return -2; //Something happened, the parameters aren't in the expected order
		}	
	}
	else {
		printf("Start and end 1st parameter are different or have different lengths\n"); 
		return -2;
	}

	//If the second parameter type is the same and have the same length in both packets
	if(packet1[3 + sizeOfFileSizeParameter1] == packet2[3 + sizeOfFileSizeParameter2] && packet1[FILENAME_SIZE_INDEX(sizeOfFileSizeParameter1)] == packet2[FILENAME_SIZE_INDEX(sizeOfFileSizeParameter2)]) {
		//And if it is a filename parameter type as expected
		if(packet1[3 + sizeOfFileSizeParameter1] == CONTROL_TYPE_FILENAME) {
			size_t fileNameSize1 = packet1[FILENAME_SIZE_INDEX(sizeOfFileSizeParameter1)];
			size_t fileNameSize2 = packet2[FILENAME_SIZE_INDEX(sizeOfFileSizeParameter2)];
			packet1FileName = malloc(fileNameSize1); 
			packet2FileName = malloc(fileNameSize2);
			if(packet1FileName == NULL || packet2FileName == NULL) {
				printf("Failed to allocate memory for packet file name, terminating\n");
				return -1;
			}

			strcpy(packet1FileName, (char*) &packet1[FILENAME_INDEX(sizeOfFileSizeParameter1)]);
			strcpy(packet2FileName, (char*) &packet2[FILENAME_INDEX(sizeOfFileSizeParameter2)]);

			//Check if the filenames are the same
			if(strcmp(packet1FileName, packet2FileName) != 0) {
				printf("Start and end packets filenames are different!\n");
				free(packet1FileName);
				free(packet2FileName);
				return -1;
			}

			free(packet1FileName);
			free(packet2FileName);
		}
		else {
			printf("Start and end parameters aren't in the expected order\n");
			return -2; //Something happened, the parameters aren't in the expected order
		}
	}
	else {
		printf("Start and end 2nd parameter are different or have different lengths\n"); 
		return -2;
	}

	return 0;
}

unsigned char* readFile(char *fileName, size_t* fileSize) {
	FILE* file;
	unsigned char* buffer;

	file = fopen(fileName, "rb");
	if(!file) {
		fprintf(stderr, "Unable to open file %s, does it exist?\n", fileName);
		return NULL;
	}
	
	fseek(file, 0, SEEK_END);
	*fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	buffer = (unsigned char *) malloc(*fileSize+1);
	if (!buffer) {
		fprintf(stderr, "Memory error!");
        fclose(file);
		return NULL;
	}

	fread(buffer, *fileSize, 1, file);
	fclose(file);

	return buffer;
}

int writeFile(unsigned char* fileBuffer, char* fileName, size_t fileSize) {
	FILE* file;

	file = fopen(fileName, "wb");
	if(!file) {
		fprintf(stderr, "Unable to create file %s!\n", fileName);
		return -1;
	}

	unsigned long result = fwrite(fileBuffer, sizeof(unsigned char), fileSize, file);
	fclose(file);
	if(result == fileSize)
		return 0;
	else {
		printf("Wrote %lu bytes when size was %lu bytes, failed\n", result, fileSize);
		return -1;
	}
}
