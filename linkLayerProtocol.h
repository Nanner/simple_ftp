#ifndef _LINKLAYERPROTOCOL_H
#define _LINKLAYERPROTOCOL_H

#define TRANSMITTER 1
#define RECEIVER 0

#define COM1 1
#define COM2 2
#define COM1_PORT "/dev/ttyS0"
#define COM2_PORT "/dev/ttyS1"
#define BAUDRATE B9600

#define FALSE 0
#define TRUE 1

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "frame.h"
#include "applicationLayer.h"

typedef struct {
    char port[20];
    int baudRate;
    unsigned int sequenceNumber;
    unsigned int timeout;
    unsigned int numTransmissions;
} LinkLayer;

extern LinkLayer linkLayerConf;

extern int retryCounter;

int llopen(int port, int role);

int llclose(int fd);

int llread(int fd, char * buffer);

int llwrite(int fd, char * buffer, int length);

int toPhysical(Frame* frame);

int fromPhysical(Frame* frame, int exitOnTimeout);

void timeout();

#endif