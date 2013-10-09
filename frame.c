#include "frame.h"

int retryCounter = 0;

Frame createSupervisionFrame(char address, char control) {
	Frame frame;
	frame.frameHeader = FRAMEFLAG;
	frame.address = address;
	frame.control = control;
	frame.bcc1 = createBCC1(address, control);
	frame.frameTrailer = FRAMEFLAG;

	return frame;
}

Frame createInfoFrame(char address, char control, char information[], int infoLength) {
	Frame frame;
	frame.frameHeader = FRAMEFLAG;
	frame.address = address;
	frame.control = control;
	frame.bcc1 = createBCC1(address, control);
	frame.frameTrailer = FRAMEFLAG;

	unsigned int i = 0;
	for(; i < MAX_INFO_LENGTH; i++)
		frame.information[i] = 0;

	copyInfo(frame.information, information, infoLength);
	frame.bcc2 = createBCC2(information);

	return frame;
}

int copyInfo(char destination[MAX_INFO_LENGTH], char source[], unsigned int length) {
	if(length > MAX_INFO_LENGTH)
		return -1;

	unsigned int i = 0;
	for(; i < length; i++) {
		destination[i] = source[i];
	}

	return 0;
}

char createBCC1(char address, char control) {
	return address ^ control;
}
char createBCC2(char information[MAX_INFO_LENGTH]) {
	char bcc = 0;
	unsigned int i = 0;
	for(; i < MAX_INFO_LENGTH; i++) {
		bcc ^= information[i];
	}

	return bcc;
}

int validBCC1(Frame frame) {
	return(createBCC1(frame.address, frame.control) == frame.bcc1);
}

int validBCC2(Frame frame) {
	return(createBCC2(frame.information) == frame.bcc2);
}

