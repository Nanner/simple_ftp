#include "supervisionFrame.h"

retryCounter = 1;

SupervisionFrame createFrame(char address, char control) {
	SupervisionFrame frame;
	frame.frameHeader = FRAMEFLAG;
	frame.address = address;
	frame.control = control;
	frame.bcc = address ^ control;
	frame.frameTrailer = FRAMEFLAG;

	return frame;
}

void timeout() {
	printf("Retry #%d\n", retryCounter);
	retryCounter++;
}
