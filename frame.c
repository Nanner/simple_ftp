#include "frame.h"

int retryCounter = 0;

char* createSupervisionFrame(char address, char control, size_t maxInformationSize) {
	char* frame = malloc(BASE_FRAME_SIZE + maxInformationSize);
	frame[FHEADERFLAG] = FRAMEFLAG;
	frame[FADDRESS] = address;
	frame[FCONTROL] = control;
	frame[FBCC1] = createBCC1(address, control);
	frame[FTRAILERFLAG(maxInformationSize)] = FRAMEFLAG;

	return frame;
}

char* createInfoFrame(char address, char control, char information[], size_t infoLength, size_t maxInformationSize) {
	char* frame = malloc(BASE_FRAME_SIZE + maxInformationSize);
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

int copyInfo(char destinationFrame[], char source[], size_t length, size_t maxInformationSize) {
	if(length > maxInformationSize)
		return -1;

	unsigned int i = 0;
	for(; i < length; i++) {
		destinationFrame[FDATA + i] = source[i];
	}

	return 0;
}

void getInfo(char* frame, char destination[], size_t length) {
	unsigned int i = 0;
	
	for(; i < length; i++) {
		destination[i] = frame[FDATA + i];
	}
}

char createBCC1(char address, char control) {
	return address ^ control;
}
char createBCC2(char information[], size_t maxInformationSize) {
	char bcc = 0;
	unsigned int i = 0;
	for(; i < maxInformationSize; i++) {
		bcc ^= information[i];
	}

	return bcc;
}

int validBCC1(char* frame) {
	return(createBCC1(frame[FADDRESS], frame[FCONTROL]) == frame[FBCC1]);
}

int validBCC2(char* frame, size_t maxInformationSize) {
	char info[maxInformationSize];
	memcpy(info, &frame[FDATA], maxInformationSize);
	printf("Info: %s\n", info);
	printf("Obtained bcc2: %x\n", createBCC2(info, maxInformationSize));
	printf("Frame bcc2: %x\n", frame[FBCC2(maxInformationSize)]);
	printf("Info size: %lu\n", strlen(info));
	return(createBCC2(info, maxInformationSize) == frame[FBCC2(maxInformationSize)]);
}

int checkForErrors(char* frame, size_t maxInformationSize, int role) {
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

void flipbit(char* byte, unsigned bitNumber){
    *byte ^= (1UL << bitNumber);
}
