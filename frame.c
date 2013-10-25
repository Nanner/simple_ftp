#include "frame.h"

int retryCounter = 0;

unsigned char* createSupervisionFrame(unsigned char address, unsigned char control, size_t maxInformationSize) {
	unsigned char* frame = malloc(BASE_FRAME_SIZE + maxInformationSize);
	if(!frame) {
		printf("Failed to allocate memory for a frame, terminating\n");
		return NULL;
	}
	frame[FHEADERFLAG] = FRAMEFLAG;
	frame[FADDRESS] = address;
	frame[FCONTROL] = control;
	frame[FBCC1] = createBCC1(address, control);
	frame[FTRAILERFLAG(maxInformationSize)] = FRAMEFLAG;

	return frame;
}

unsigned char* createInfoFrame(unsigned char address, unsigned char control, unsigned char information[], size_t infoLength, size_t maxInformationSize) {
	unsigned char* frame = malloc(BASE_FRAME_SIZE + maxInformationSize);
	if(!frame) {
		printf("Failed to allocate memory for a frame, terminating\n");
		return NULL;
	}
	
	frame[FHEADERFLAG] = FRAMEFLAG;
	frame[FADDRESS] = address;
	frame[FCONTROL] = control;
	frame[FBCC1] = createBCC1(address, control);
	frame[FTRAILERFLAG(maxInformationSize)] = FRAMEFLAG;

	unsigned int i = 0;
	for(; i < maxInformationSize; i++)
		frame[FDATA + i] = 0;

	copyInfo(frame, information, infoLength, maxInformationSize);
	frame[FBCC2(maxInformationSize)] = createBCC2(information, infoLength);

	return frame;
}

int copyInfo(unsigned char destinationFrame[], unsigned char source[], size_t length, size_t maxInformationSize) {
	if(length > maxInformationSize)
		return -1;

	unsigned int i = 0;
	for(; i < length; i++) {
		destinationFrame[FDATA + i] = source[i];
	}

	return 0;
}

void getInfo(unsigned char* frame, unsigned char destination[], size_t length) {
	unsigned int i = 0;
	
	for(; i < length; i++) {
		destination[i] = frame[FDATA + i];
	}
}

unsigned char createBCC1(unsigned char address, unsigned char control) {
	return address ^ control;
}
unsigned char createBCC2(unsigned char information[], size_t maxInformationSize) {
	unsigned char bcc = 0;
	unsigned int i = 0;
	for(; i < maxInformationSize; i++) {
		bcc ^= information[i];
	}

	return bcc;
}

int validBCC1(unsigned char* frame) {
	char obcc1[20];
	sprintf(obcc1, "Calculated BCC1: %X\n", createBCC1(frame[FADDRESS], frame[FCONTROL]));
	writeToLog(obcc1);

	return(createBCC1(frame[FADDRESS], frame[FCONTROL]) == frame[FBCC1]);
}

int validBCC2(unsigned char* frame, size_t maxInformationSize) {
	unsigned char info[maxInformationSize];
	memcpy(info, &frame[FDATA], maxInformationSize);

	char obcc2[20];
	sprintf(obcc2, "Calculated BCC2: %X\n", createBCC2(info, maxInformationSize));
	writeToLog(obcc2);

	return(createBCC2(info, maxInformationSize) == frame[FBCC2(maxInformationSize)]);
}

int checkForErrors(unsigned char* frame, size_t maxInformationSize, int role) {
	//Check header and trailer fields
	if(frame[FHEADERFLAG] != FRAMEFLAG || frame[FTRAILERFLAG(maxInformationSize)] != FRAMEFLAG)
		return FRAME_HEADER_ERROR;

	int infoFrame = 0;
	int commandFrame = 0;
	//Check control field
	if(frame[FCONTROL] == INFO_0 || frame[FCONTROL] == INFO_1) {
		infoFrame = 1;
		commandFrame = 1;
	}
	else if(frame[FCONTROL] == SET || frame[FCONTROL] == DISC) {
		infoFrame = 0;
		commandFrame = 1;
	}
	else if(frame[FCONTROL] == UA ||
		frame[FCONTROL]	== RR_0 ||
		frame[FCONTROL] == RR_1 ||
		frame[FCONTROL] == REJ_0 ||
		frame[FCONTROL] == REJ_1) {
		infoFrame = 0;
		commandFrame = 0;
	}
	else
		return FRAME_HEADER_ERROR;

	//Check if address is what's expected
	if(role == TRANSMITTER && commandFrame && frame[FADDRESS] != SENDER_ADDRESS)
		return FRAME_HEADER_ERROR;
	else if(role == TRANSMITTER && !commandFrame && frame[FADDRESS] != RECEIVER_ADDRESS)
		return FRAME_HEADER_ERROR;
	else if(role == RECEIVER && commandFrame && frame[FADDRESS] != RECEIVER_ADDRESS)
		return FRAME_HEADER_ERROR;
	else if(role == RECEIVER && !commandFrame && frame[FADDRESS] != SENDER_ADDRESS)
		return FRAME_HEADER_ERROR;

	//Check BCCs
	if(!validBCC1(frame))
		return FRAME_HEADER_ERROR;

	if(infoFrame) {
		if(!validBCC2(frame, maxInformationSize))
			return FRAME_INFO_ERROR;
	}

	return 0;
}

void flipbit(unsigned char* byte, unsigned bitNumber){
    *byte ^= (1UL << bitNumber);
}
