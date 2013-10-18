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
    linkLayerConf.frameTrailerIndex = FTRAILER(linkLayerConf.maxInformationSize);
    printf("maxInfoSize: %lu\n", linkLayerConf.maxInformationSize);
    
    int fd;

    fd = llopen(port, role);
    
    llclose(fd);
    
    return 0;
}