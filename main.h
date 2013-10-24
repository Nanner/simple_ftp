#ifndef _MAIN_H
#define _MAIN_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "linkLayerProtocol.h"
#include "applicationLayer.h"

#define MINIMUM_DATA_SIZE 90

#define DEFAULT_DATA_SIZE 128
#define DEFAULT_BAUDRATE B9600
#define DEFAULT_BAUDRATE_STRING "B9600"
#define DEFAULT_TIMEOUT 3
#define DEFAULT_RETRY 3

int main(int argc, char** argv);

int setBaudrate(char * baudrateString);
int setDataSize(char * dataSizeString);
int setRetry(char * retryNumberString);
int setTimeout(char * secondsString);

#endif