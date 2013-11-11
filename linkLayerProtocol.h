#ifndef _LINKLAYERPROTOCOL_H
#define _LINKLAYERPROTOCOL_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include "frame.h"
#include "applicationLayer.h"

#define TRANSMITTER 1
#define RECEIVER 0

#define COM1 1
#define COM2 2
#define COM1_PORT "/dev/ttyS0"
#define COM2_PORT "/dev/ttyS1"

#define MINIMUM_DATA_SIZE 90

#define DEFAULT_DATA_SIZE 128
#define DEFAULT_BAUDRATE B9600
#define DEFAULT_BAUDRATE_STRING "B9600"
#define DEFAULT_TIMEOUT 3
#define DEFAULT_RETRY 3

#define FALSE 0
#define TRUE 1

#define SET_UA_TIMEOUT 30
#define RECEIVE_INFO_TIMEOUT 30

//Byte stuffing codes
#define ESCAPE_BYTE 0x7D
#define ESCAPED_FLAG 0x5E
#define ESCAPED_ESCAPE 0x5D
#define XOR_BYTE 0x20

#define LOGNAME_MAX_LEN 512
#define LOG_FILE ".log"
#define MESSAGE_LEN 1024
#define SENT 1
#define RECEIVED 2
#define TIME_LEN 23

//Test mode constants
#define ERROR_CHANCE 20
#define INFO_ERROR_CHANCE 50

typedef struct {
    char port[20];
    speed_t baudRate;
    unsigned int sequenceNumber;
    unsigned int sendTimeout;
    unsigned int receiveTimeout;
    unsigned int numTransmissions;
    struct termios oldtio;
    size_t maxInformationSize;
    size_t frameSize;
    unsigned long frameBCC2Index;
    unsigned long frameTrailerIndex;
    char logname[LOGNAME_MAX_LEN];
    unsigned int testMode;
} LinkLayer;

extern LinkLayer linkLayerConf;

extern int retryCounter;

int llopen(int port, int role);

int llclose(int fd);

int setBaudrate(char * baudrateString);

int setDataSize(char * dataSizeString);

int setRetry(char * retryNumberString);

int setTimeout(char * secondsString);

int setTestMode(char * testString);

int receiveData(unsigned char* packet, size_t packetLength);

int sendData(unsigned char* packet, size_t packetLength);

int setLink();

int waitForLink();

int closeLink();

int waitCloseLink();

int confirmCloseLink();

void timeout();

void initializeLog(char * logname);
void writeToLog(char * string);
void writeFrameToLog(unsigned char * frame, int direction);


#endif
