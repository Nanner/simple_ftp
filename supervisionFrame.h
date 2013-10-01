#ifndef _SUPERVISIONFRAME_H
#define _SUPERVISIONFRAME_H

#include <stdio.h>

#define RECEIVER_ADDRESS 0x03
#define SENDER_ADDRESS 0x01

#define SET 0x03
#define DISC 0x0B
#define UA 0x07

#define FRAMEFLAG 0x7E

typedef struct {
    char frameHeader;
    char address;
    char control;
    char bcc;
    char frameTrailer;
} SupervisionFrame;

SupervisionFrame createFrame(char address, char control);

void timeout();

extern int retryCounter;

#endif
