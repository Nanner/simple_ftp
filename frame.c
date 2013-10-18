#include "frame.h"

int retryCounter = 0;

char* createSupervisionFrame(char address, char control, size_t maxInformationSize) {
	char* frame = malloc(BASE_FRAME_SIZE + maxInformationSize);
	frame[FHEADER] = FRAMEFLAG;
	frame[FADDRESS] = address;
	frame[FCONTROL] = control;
	frame[FBCC1] = createBCC1(address, control);
	frame[FTRAILER(maxInformationSize)] = FRAMEFLAG;

	return frame;
}

char* createInfoFrame(char address, char control, char information[], size_t infoLength, size_t maxInformationSize) {
	char* frame = malloc(BASE_FRAME_SIZE + maxInformationSize);
	frame[FHEADER] = FRAMEFLAG;
	frame[FADDRESS] = address;
	frame[FCONTROL] = control;
	frame[FBCC1] = createBCC1(address, control);
	frame[FTRAILER(maxInformationSize)] = FRAMEFLAG;

	unsigned int i = 0;
	for(; i < maxInformationSize; i++)
		frame[FDATA + i] = 0;

	copyInfo(frame, information, infoLength, maxInformationSize);
	frame[FBCC2(maxInformationSize)] = createBCC2(information, maxInformationSize);

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
	return(createBCC2(info, maxInformationSize) == frame[FBCC2(maxInformationSize)]);
}

