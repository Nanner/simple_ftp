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

    linkLayerConf.maxInformationSize = atoi(argv[3]);
    linkLayerConf.frameSize = linkLayerConf.maxInformationSize + BASE_FRAME_SIZE;
    linkLayerConf.frameBCC2Index = FBCC2(linkLayerConf.maxInformationSize);
    linkLayerConf.frameTrailerIndex = FTRAILERFLAG(linkLayerConf.maxInformationSize);
    printf("maxInfoSize: %lu\n", (unsigned long) linkLayerConf.maxInformationSize);

    //TODO this is temporary, we should ask the user to define this in the future
    linkLayerConf.numTransmissions = 4; //first transmission + retries
    linkLayerConf.timeout = 3; //seconds until timeout
    
    int fd;

    fd = llopen(port, role);
    //TODO main receiving/sending loop

    if(role == TRANSMITTER) {
        //TODO these will be changed by the application layer later on, not fixed
        int numberOfPackets = 4;
        char packetArray[4][256] = {"cookies", "chocolate", "chocolatefruitsareamazingstuffdude", "annoyinglastpacket"};

        unsigned int i = 0;
        for(; i < numberOfPackets; i++) {
            printf("Sending packet %d\n", i);
            if(sendPacket(packetArray[i], 256) != -1)
                printf("Sent packet %d\n", i);
        }

        //disconnect();
    }
    else if(role == RECEIVER) {
        int numberOfPackets = 4; //TODO temporary, this needs to be figured out from the packets themselves, I think

        unsigned int i = 0;
        for(; i < numberOfPackets; i++) {
            char* string = malloc(256);
            int res = receivePacket(string, 256);
            if(res != -1)
                printf("Received %d bytes: %s\n", res, string);
            else
                printf("Reception failed\n");
        }

    }

    llclose(fd);
    
    return 0;
}