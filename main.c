#include "main.h"

LinkLayer linkLayerConf;
ApplicationLayer applicationLayerConf;

int main(int argc, char** argv)
{    
    if ( (argc < 4) ||
        ((strcmp("/dev/ttyS0", argv[1])!=0) &&
         (strcmp("/dev/ttyS1", argv[1])!=0) ) ||
        ((strcmp("transmitter",argv[2])!=0) &&
         (strcmp("receiver",   argv[2])!=0)  )  ) {
            printf("Usage:\tnserial SerialPort transmitter|receiver maxInformationFieldSize\n\tex: nserial /dev/ttyS1 transmitter 256\n");
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

    applicationLayerConf.maxPacketSize = atoi(argv[3]);
    applicationLayerConf.maxDataFieldSize = applicationLayerConf.maxPacketSize - BASE_DATA_PACKET_SIZE;
    linkLayerConf.maxInformationSize = applicationLayerConf.maxPacketSize * 2 + 4;
    linkLayerConf.frameSize = linkLayerConf.maxInformationSize + BASE_FRAME_SIZE;
    linkLayerConf.frameBCC2Index = FBCC2(linkLayerConf.maxInformationSize);
    linkLayerConf.frameTrailerIndex = FTRAILERFLAG(linkLayerConf.maxInformationSize);
    printf("maxInfoSize: %lu\n", (unsigned long) linkLayerConf.maxInformationSize);

    //TODO this is temporary, we should ask the user to define this in the future
    linkLayerConf.numTransmissions = 4; //first transmission + retries
    linkLayerConf.sendTimeout = 3; //seconds until timeout
    linkLayerConf.receiveTimeout = RECEIVE_INFO_TIMEOUT;
    
    int fd;

    fd = llopen(port, role);
    //TODO main receiving/sending loop

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
        char* fileName = "/home/nanner/virtualShared/pinguim.gif";
        size_t fileSize;
        char* file = readFile(fileName, &fileSize);
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
        char* file = receiveFile(&size, fileName);
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