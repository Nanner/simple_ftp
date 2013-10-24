#include "main.h"

LinkLayer linkLayerConf;
ApplicationLayer applicationLayerConf;

int main(int argc, char** argv)
{    
    if ( (argc < 3) ||
        ((strcmp("/dev/ttyS0", argv[1])!=0) &&
         (strcmp("/dev/ttyS1", argv[1])!=0) ) ||
        ((strcmp("transmitter",argv[2])!=0) &&
         (strcmp("receiver",   argv[2])!=0)  )  ) {
            printf("Usage:\tnserial SerialPort transmitter|receiver [-b baudrate] [-s size of data per packet] [-r retry number] [-t timeout seconds]\n");
            printf("Example:\tnserial /dev/ttyS0 transmitter -b B115200 -s 1024 -r 5 -t 20\n");
            exit(1);
    }
    
    int port = 0, role = 0;

    if (strcmp(argv[1], COM1_PORT) == 0 )
        port = COM1;
    else if (strcmp(argv[1], COM2_PORT) == 0)
        port = COM2;

    if ( strcmp("transmitter", argv[2]) == 0 )
        role = TRANSMITTER;

    if ( strcmp("receiver", argv[2]) == 0 )
        role = RECEIVER;

    // default settings
    applicationLayerConf.maxPacketSize = DEFAULT_DATA_SIZE;
    applicationLayerConf.maxDataFieldSize = applicationLayerConf.maxPacketSize - BASE_DATA_PACKET_SIZE;
    linkLayerConf.maxInformationSize = applicationLayerConf.maxPacketSize * 2 + 4;
    linkLayerConf.frameSize = linkLayerConf.maxInformationSize + BASE_FRAME_SIZE;
    
    linkLayerConf.frameBCC2Index = FBCC2(linkLayerConf.maxInformationSize);
    linkLayerConf.frameTrailerIndex = FTRAILERFLAG(linkLayerConf.maxInformationSize);

    linkLayerConf.numTransmissions = 1 + DEFAULT_RETRY; //first transmission + retries
    linkLayerConf.sendTimeout = DEFAULT_TIMEOUT; //seconds until timeout
    linkLayerConf.receiveTimeout = RECEIVE_INFO_TIMEOUT;
    linkLayerConf.baudRate = B9600;
    
    int c; opterr = 0;
    int result;
    
    while ((c = getopt (argc, argv, "b:s:r:t:")) != -1)
        switch (c)
    {
        case 'b':
            result = setBaudrate(optarg);
            if ( result == 0 )
                printf("Baud rate set to %s\n", optarg);
            else
                printf("Error setting baud rate! Reverted to default %s\n", DEFAULT_BAUDRATE_STRING);
            break;
        case 's':
            result = setDataSize(optarg);
            if ( result == 0)
                printf("Size of data per packet set to %s\n", optarg);
            else
                printf("Error setting size of data per packet! Reverting to default %i\n", DEFAULT_DATA_SIZE);
            break;
        case 'r':
            result = setRetry(optarg);
            if ( result == 0)
                printf("Number of retries set to %s\n", optarg);
            else
                printf("Error setting number of retries! Reverting to default %i\n", DEFAULT_RETRY);
            break;
        case 't':
            result = setTimeout(optarg);
            if ( result == 0)
                printf("Timeout set to %s seconds", optarg);
            else
                printf("Error setting timeout seconds! Reverting to default %i", DEFAULT_TIMEOUT);
            break;
        case '?':
            break;
        default:
            return -1;
    }
    
    printf("maxInfoSize: %lu\n", (unsigned long) linkLayerConf.maxInformationSize);
    
    int fd;
    
    fd = llopen(port, role);
    
    // TODO restrict the minimum packet size
    // TODO funcoes de set ao protocol
    // 

    if(role == TRANSMITTER && fd != -1) {
        //TODO these will be changed by the application layer later on, not fixed
        /*int numberOfPackets = 4;
        char packetArray[4][256] = {"cookies", "choco}~late", "chocolate~fruits~are~amazing~stuff~dude", "annoying}last}packet"};

        unsigned int i = 0;
        for(; i < numberOfPackets; i++) {
            printf("Sending packet %d\n", i);
            if(sendPacket(packetArray[i], 256) != -1)
                printf("Sent packet %d\n", i);
            else
                break;
        }*/
        char* fileName = "./pinguim.gif";
        size_t fileSize;
        unsigned char* file = readFile(fileName, &fileSize);
        //char* gibberishFile = "I'm a test, a little little test, I wonder if this will work, this will probably not work, oh well. Spam.";
        if(sendFile(file, fileSize, fileName) == 0)
            printf("Sent.");

        closeLink();
    }
    else if(role == RECEIVER && fd != -1) {
        //char* gibberishFile = "I'm a test, a little little test, I wonder if this will work, this will probably not work, oh well. Spam.";
        /*int numberOfPackets = 3; //TODO temporary, this needs to be figured out from the packets themselves, I think

        unsigned int i = 0;
        for(; i < numberOfPackets; i++) {
            char* string = malloc(128);
            int res = receivePacket(string, 128);
            if(res == -5)
                printf("Link was closed unexpectedly");
            else if(res != -1) {
                if(i == 1)
                    printf("Received %d bytes: %s\n", res, &string[DATA_INDEX]);
                else
                    printf("Received packet\n");
            }
            else
                printf("Reception failed\n");
            
            free(string);
        }*/
        size_t size;
        char fileName[applicationLayerConf.maxPacketSize - (BASE_DATA_PACKET_SIZE + sizeof(size_t))];
        unsigned char* file = receiveFile(&size, fileName);
        char* newFileName = "./notAPenguin.gif";
        if(writeFile(file, newFileName, size) == 0)
            printf("Success! File should be created!\n");
        else
            printf("Failed to create file\n");
        //printf("Received: %s\n", file);

        waitCloseLink();
    }
    
    return 0;
}

int setBaudrate(char * baudrateString){
    speed_t rates[] = {B0, B110, B115200, B1200, B134, B150, B1800, B19200, B200, B230400, B2400, B300, B38400, B4800, B50, B57600, B600, B75, B9600};
    char * rateStrings[]= {"B0", "B110", "B115200", "B1200", "B134", "B150", "B1800", "B19200", "B200", "B230400", "B2400", "B300", "B38400", "B4800", "B50", "B57600", "B600", "B75", "B9600"};
    int ratesSize = 19;
    
    int i;
    for (i = 0; i < ratesSize; i++) {
        if (strcmp(baudrateString, rateStrings[i]) == 0 ) {
            linkLayerConf.baudRate = rates[i];
            return 0;
        }
    }
    
    return 0;
}

int setDataSize(char * dataSizeString){
    int size = atoi(dataSizeString);
    
    if ( size < MINIMUM_DATA_SIZE )
        return -1;
    
    applicationLayerConf.maxPacketSize = size;
    applicationLayerConf.maxDataFieldSize = applicationLayerConf.maxPacketSize - BASE_DATA_PACKET_SIZE;
    linkLayerConf.maxInformationSize = applicationLayerConf.maxPacketSize * 2 + 4;
    linkLayerConf.frameSize = linkLayerConf.maxInformationSize + BASE_FRAME_SIZE;
    
    linkLayerConf.frameBCC2Index = FBCC2(linkLayerConf.maxInformationSize);
    linkLayerConf.frameTrailerIndex = FTRAILERFLAG(linkLayerConf.maxInformationSize);
    
    return 0;
}

int setRetry(char * retryNumberString){
    int numberOfRetries = atoi(retryNumberString);
                                //first transmission + retries
    linkLayerConf.numTransmissions = 1 + numberOfRetries;
    return 0;
}

int setTimeout(char * secondsString){
    int seconds = atoi(secondsString);
    linkLayerConf.sendTimeout = seconds; //seconds until timeout
    return 0;
}