#ifndef _SUPERVISIONFRAME_H
#define _SUPERVISIONFRAME_H

#include <stdio.h>

#define MAX_INFO_LENGTH 256

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
    char information[MAX_INFO_LENGTH];
    char bcc1;
    char bcc2;
    char frameTrailer;
} Frame;

Frame createSupervisionFrame(char address, char control);
Frame createInfoFrame(char address, char control, char information[], int infoLength);

int copyInfo(char destination[MAX_INFO_LENGTH], char source[], unsigned int length);

char createBCC1(char address, char control);
char createBCC2(char information[MAX_INFO_LENGTH]);
int validBCC1(Frame frame);
int validBCC2(Frame frame);

#endif
