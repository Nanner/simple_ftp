#include "main.h"

LinkLayer linkLayerConf;
ApplicationLayer applicationLayerConf;

//TODO allow user to specify filename

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

    if(role == TRANSMITTER && fd != -1) {
        char* fileName = "./pinguim.gif";
        size_t fileSize;
        unsigned char* file = readFile(fileName, &fileSize);
        //char* gibberishFile = "I'm a test, a little little test, I wonder if this will work, this will probably not work, oh well. Spam.";
        if(sendFile(file, fileSize, fileName) == 0)
            printf("Sent.");

        closeLink();
    }
    else if(role == RECEIVER && fd != -1) {
        size_t size;

        char fileName[applicationLayerConf.maxPacketSize - (BASE_DATA_PACKET_SIZE + sizeof(size_t))];

        unsigned char* file = receiveFile(&size, fileName);
        if(file != NULL) {
            char* newFileName = "./notAPenguin.gif";

            if(writeFile(file, newFileName, size) == 0)
                printf("Success! File should be created!\n");
            else
                printf("Failed to create file\n");

            waitCloseLink();
        }
        else {
            printf("Failed to correctly receive file\n");
        }
        
    }
    
    return 0;
}