#include "supervisionFrame.h"

int retryCounter = 0;

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
    retryCounter++;
}
