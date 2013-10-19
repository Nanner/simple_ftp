#include "linkLayerProtocol.h"

int llopen(int port, int role){

    struct termios newtio;
    /*
     Open serial port device for reading and writing and not as controlling tty
     because we don't want to get killed if linenoise sends CTRL-C.
     */

    if ( port == COM1 ) {
    	applicationLayerConf.fileDescriptor = open(COM1_PORT, O_RDWR | O_NOCTTY );
    } else if ( port == COM2 ) {
    	applicationLayerConf.fileDescriptor = open(COM2_PORT, O_RDWR | O_NOCTTY );
    } else applicationLayerConf.fileDescriptor = -1;

    if (applicationLayerConf.fileDescriptor < 0) {
        perror("port error"); exit(-1); 
    }
    
    if ( tcgetattr(applicationLayerConf.fileDescriptor,&linkLayerConf.oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        return -1;
    }
    
    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    
    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    
    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */
    
    /*
     VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
     leitura do(s) próximo(s) caracter(es)
     */
    
    tcflush(applicationLayerConf.fileDescriptor, TCIOFLUSH);
    
    if ( tcsetattr(applicationLayerConf.fileDescriptor,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        return -1;
    }

    applicationLayerConf.status = role;

    if (role == TRANSMITTER) {
       int result = setLink();

       switch(result) {
        case 0:
            printf("Handshake sucess! Link set.\n");
            break;
        case -1:
            printf("Failed to set link, connection timed out.\n");
            return -1;
            break;
       }
    }

    if (role == RECEIVER)
    {
        printf("Waiting for connection...\n");
        int result = waitForLink();

        switch(result) {
        case 0:
            printf("Handshake sent, link should be set.\n");
            break;
        case -1:
            printf("Failed to set link, connection timed out.\n");
            return -1;
            break;
       }
    }

    linkLayerConf.sequenceNumber = 0;
    
    return applicationLayerConf.fileDescriptor;
}

int llclose(int fd) {

    if ( tcsetattr(applicationLayerConf.fileDescriptor,TCSANOW,&linkLayerConf.oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    
    return 0;
}

int setLink() {
    return(sendCommand(SET, UA, linkLayerConf.sendTimeout, linkLayerConf.numTransmissions, RECEIVER_ADDRESS));
}

int waitForLink() {
    char command;
    int res = receiveCommand(&command, linkLayerConf.receiveTimeout);
    printf("Command: %d\nRes: %d\n", command, res);
    if(command == SET && res != -1) {
        return(sendResponse(UA, RECEIVER_ADDRESS));
    }

    return -1;
}

int closeLink() {
    if(sendCommand(DISC, DISC, linkLayerConf.sendTimeout, linkLayerConf.numTransmissions, RECEIVER_ADDRESS) == 0)
        sendResponse(UA, SENDER_ADDRESS);

    printf("Closing link.\n");
    return(llclose(applicationLayerConf.fileDescriptor));
}

int waitCloseLink() {
    char command;
    int res = receiveCommand(&command, linkLayerConf.receiveTimeout);
    if(command == DISC && res != -1) {
        return(confirmCloseLink());
    }

    return -1;
}

int confirmCloseLink() {
    if(sendCommand(DISC, UA, linkLayerConf.receiveTimeout, 1, SENDER_ADDRESS) == 0) {
        printf("Closing link.\n");
        return(llclose(applicationLayerConf.fileDescriptor));
    }

    return -1;
}

void timeout() {
    retryCounter++;
}

int toPhysical(char* frame) {
    tcflush(applicationLayerConf.fileDescriptor, TCIOFLUSH);
    char* stuffedFrame = malloc(linkLayerConf.frameSize);
    stuffFrame(frame, stuffedFrame, linkLayerConf.frameSize, linkLayerConf.maxInformationSize);
    int res = write(applicationLayerConf.fileDescriptor, stuffedFrame, linkLayerConf.frameSize);
    free(stuffedFrame);
    return res;
}

int fromPhysical(char* frame, int exitOnTimeout) {
    int curchar = 0;
    char receivedString[linkLayerConf.frameSize];
    char buf[linkLayerConf.frameSize];
    int STOP=FALSE;
    int res = 0;
    int currentTry = retryCounter;

    while (STOP==FALSE) {
        res = read(applicationLayerConf.fileDescriptor,buf,1);
        if(res == 1){
            //If we are receiving the first byte, we want to make sure it's a frame header or a frame trailer before we start receiving the frame
            if(curchar == 0) {
                if(buf[0] == FRAMEFLAG) {
                    //So, we only accept the byte if it's a frameflag
                    receivedString[curchar] = buf[0];
                    curchar++;
                }
            //If we are on the second byte but receive yet another frameflag, it means the previous byte we received was the end of some
            //frame, and not the start of the one we want to receive. So, we just "push" our received string one byte down and start receiving
            //the rest of the frame. Since we use byte stuffing, we won't receive a frameflag in the middle of a valid frame, so we can use this.
            } else if(curchar == 1 && buf[0] == FRAMEFLAG) {
                receivedString[0] = buf[0];
            }
            else {
                receivedString[curchar] = buf[0];
                curchar++;
            }
        }
        if (receivedString[curchar-1]==FRAMEFLAG && curchar-1 > 0 && curchar == linkLayerConf.frameSize)
            STOP=TRUE;
        else if (receivedString[curchar-1] == FRAMEFLAG) {
            //Found a frame flag, so we will reset our buffer and the char count
            receivedString[0] = FRAMEFLAG;
            curchar = 1;
        }

        if(exitOnTimeout && (currentTry < retryCounter))
            return -1;
    }

    destuffFrame(receivedString, frame, linkLayerConf.frameSize);
    return curchar;
}

int receiveCommand(char* command, int tryTimeout) {
    // install timeout handler
    struct sigaction sa;
    sa.sa_handler = timeout;
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    char* receivedFrame = malloc(linkLayerConf.frameSize);

    alarm(tryTimeout);
    while (retryCounter < 1) {
        if(fromPhysical(receivedFrame, 1) != -1) {

            printf("\nReceived:\n%x %x %x %x %x\n", receivedFrame[FHEADERFLAG], receivedFrame[FADDRESS], receivedFrame[FCONTROL], receivedFrame[FBCC1], receivedFrame[linkLayerConf.frameTrailerIndex]);
            int errorCheckResult = checkForErrors(receivedFrame, linkLayerConf.maxInformationSize, applicationLayerConf.status);

            if(errorCheckResult == 0) {
                *command = receivedFrame[FCONTROL];
                free(receivedFrame);
                return 0;
            }
        }
    }

    free(receivedFrame);
    return -1;
}

int sendCommand(char command, char expectedResponse, int tryTimeout, int retries, char address) {

    // install timeout handler
    struct sigaction sa;
    sa.sa_handler = timeout;
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);


    char* frame = createSupervisionFrame(address, command, linkLayerConf.maxInformationSize);
    int responseResult;

    retryCounter = 0;

    while (retryCounter < retries) {

        if (retryCounter > 0)
            printf("Retry #%d\n", retryCounter);

        printf("Sending: %x %x %x %x %x\n", frame[FHEADERFLAG], frame[FADDRESS], frame[FCONTROL], frame[FBCC1], frame[linkLayerConf.frameTrailerIndex]);
        int res = toPhysical(frame);
        printf("%d bytes sent\n", res);

        alarm(tryTimeout);
        int currentTry = retryCounter;

        responseResult = receiveResponse(expectedResponse, currentTry);
        if(responseResult == 0)
            break;
    }

    if (responseResult == 0) {
        free(frame);
        return 0;
    }
    else {
        free(frame);
        return -1;
    }

}

int receiveResponse(char response, int currentTry) {
    int res = 0;
    char* receivedFrame = malloc(linkLayerConf.frameSize);
    while (retryCounter == currentTry) {
        printf("Waiting for response\n");
        res = fromPhysical(receivedFrame, 1);
        if(res != -1) {
            int errorCheckResult = checkForErrors(receivedFrame, linkLayerConf.maxInformationSize, applicationLayerConf.status);
            if (errorCheckResult == 0) {
                if (receivedFrame[FCONTROL] == response) {
                    alarm(0);
                    return 0;
                } else {
                    printf("Didn't receive expected response!\n");
                }
            }
            else {
                printf("Error in received frame!\n");
            }
        }
    }

    return -1;
}

int sendResponse(char response, char address) {
    char* confirmationFrame = createSupervisionFrame(address, response, linkLayerConf.maxInformationSize);
    int res = toPhysical(confirmationFrame);
    printf("Sending %d bytes: %x %x %x %x %x\n", res, confirmationFrame[FHEADERFLAG], confirmationFrame[FADDRESS], confirmationFrame[FCONTROL], confirmationFrame[FBCC1], confirmationFrame[linkLayerConf.frameTrailerIndex]);
    free(confirmationFrame);
    return 0;
}

int receivePacket(char* packet, size_t packetLength) {
    //TODO maybe accept receiving a DISC here?
    char* receivedFrame = malloc(linkLayerConf.frameSize);

    alarm(RECEIVE_INFO_TIMEOUT);
    int receivedNewPacket = 0;
    int res;
    while(!receivedNewPacket && retryCounter < 1) {
        printf("Trying to receive new packet...\n");
        res = fromPhysical(receivedFrame, 1);
        if(res != -1) {
            int errorCheckResult = checkForErrors(receivedFrame, linkLayerConf.maxInformationSize, applicationLayerConf.status);

            if(errorCheckResult == 0) {
                printf("No errors found in this frame...\n");
                //If we received the expected frame
                if(receivedFrame[FCONTROL] == linkLayerConf.sequenceNumber) {
                    printf("It's a new packet!\n");
                    linkLayerConf.sequenceNumber ^= INFO_1;
                    getInfo(receivedFrame, packet, packetLength);
                    receivedNewPacket = 1;
                }

                unsigned int rr;
                if(linkLayerConf.sequenceNumber == INFO_0)
                    rr = RR_0;
                else
                    rr = RR_1;
                char* rrFrame = createSupervisionFrame(RECEIVER_ADDRESS, rr, linkLayerConf.maxInformationSize);
                toPhysical(rrFrame);
            }
            else if(errorCheckResult == FRAME_INFO_ERROR) {
                printf("Found info error in this frame...\nFrame BCC2: %x\n", receivedFrame[FBCC2(linkLayerConf.maxInformationSize)]);
                //If this was the expected frame, we want to reject it so that the sender can resend the frame earlier
                if(receivedFrame[FCONTROL] == linkLayerConf.sequenceNumber) {
                    unsigned int rej;
                    if(linkLayerConf.sequenceNumber == INFO_0)
                        rej = REJ_0;
                    else
                        rej = REJ_1;
                    char* rejFrame = createSupervisionFrame(RECEIVER_ADDRESS, rej, linkLayerConf.maxInformationSize);
                    toPhysical(rejFrame);
                }
                //If this was a repeated frame, we want to send an rr so that the sender resends the frame earlier
                else {
                    unsigned int rr;
                    if(linkLayerConf.sequenceNumber == INFO_0)
                        rr = RR_0;
                    else
                        rr = RR_1;
                    char* rrFrame = createSupervisionFrame(RECEIVER_ADDRESS, rr, linkLayerConf.maxInformationSize);
                    toPhysical(rrFrame);
                }
            }
            else
                printf("Frame messed up\n");
        }
        else
            printf("Timed out\n");

    }

    if(receivedNewPacket) {
        printf("Received: %s\n", packet);
        return res;
    }
    else
        return -1;
}

int sendPacket(char* packet, size_t packetLength) {
    if(packetLength > linkLayerConf.maxInformationSize) {
        printf("Packet is too big for defined information field size!\n");

    }
    char* frame = createInfoFrame(RECEIVER_ADDRESS, linkLayerConf.sequenceNumber, packet, packetLength, linkLayerConf.maxInformationSize);

    int STOP=FALSE;
    char* receivedFrame = malloc(linkLayerConf.frameSize);
    int res;
    retryCounter = 0;

    int sendNextRR, resendRR, expectedREJ;
    if(linkLayerConf.sequenceNumber == INFO_0) {
        sendNextRR = RR_1;
        resendRR = RR_0;
        expectedREJ = REJ_0;
    }
    else {
        sendNextRR = RR_0;
        resendRR = RR_1;
        expectedREJ = REJ_1;
    }

    while (retryCounter < linkLayerConf.numTransmissions) {

        if (retryCounter > 0)
            printf("Retry #%d\n", retryCounter);

        res = toPhysical(frame);
        printf("%d bytes sent\n", res);

        alarm(linkLayerConf.sendTimeout);
        int receivedResend = 0;
        int currentTry = retryCounter;

        while (STOP == FALSE && retryCounter == currentTry && !receivedResend) {
            printf("Waiting for acknowledgement...\n");
            res = fromPhysical(receivedFrame, 1);
            if(res != -1) {
                int errorCheckResult = checkForErrors(receivedFrame, linkLayerConf.maxInformationSize, applicationLayerConf.status);
                if (errorCheckResult == 0){
                    if (receivedFrame[FCONTROL] == sendNextRR) {
                        alarm(0);
                        printf("Received positive acknowledgement, send next frame\n");
                        linkLayerConf.sequenceNumber ^= INFO_1;
                        STOP = TRUE;
                    } else if (receivedFrame[FCONTROL] == resendRR || receivedFrame[FCONTROL] == expectedREJ) { 
                        printf("Asked for resend!\n");
                        alarm(0);
                        retryCounter = 0;
                        receivedResend = 1;
                    }
                }
                else {
                    printf("Error in received frame!\n");
                }
            }
        }
        if(STOP == TRUE)
            break;
    }

    if (STOP == TRUE) {
        printf("Sent frame and received acknowledgement\n");
        return res;
        free(frame);
        free(receivedFrame);
        return 0;
    }
    else {
        printf("Failed to receive acknowledgement\n");
        free(frame);
        free(receivedFrame);
        return -1;
    }

}

void stuffFrame(char* destuffedFrame, char* stuffedFrame, size_t frameSize, size_t maxInformationSize) {
    unsigned int currentDestuffedByte = 1;
    unsigned int currentStuffedByte = 1;

    stuffedFrame[0] = FRAMEFLAG;
    stuffedFrame[frameSize - 1] = FRAMEFLAG;

    for(; currentStuffedByte < frameSize - 1; currentDestuffedByte++, currentStuffedByte++) {
        if(destuffedFrame[currentDestuffedByte] == FRAMEFLAG) {
            stuffedFrame[currentStuffedByte] = ESCAPE_BYTE;
            currentStuffedByte++;
            stuffedFrame[currentStuffedByte] = ESCAPED_FLAG;
        }
        else if(destuffedFrame[currentDestuffedByte] == ESCAPE_BYTE) {
            stuffedFrame[currentStuffedByte] = ESCAPE_BYTE;
            currentStuffedByte++;
            stuffedFrame[currentStuffedByte] = ESCAPED_ESCAPE;
        }
        else {
            stuffedFrame[currentStuffedByte] = destuffedFrame[currentDestuffedByte];
        }
    }

    unsigned int bcc2Position = FBCC2(maxInformationSize);
    if(destuffedFrame[bcc2Position] == FRAMEFLAG) {
        stuffedFrame[bcc2Position - 1] = ESCAPE_BYTE;
        stuffedFrame[bcc2Position] = ESCAPED_FLAG;
    }
    else if(destuffedFrame[bcc2Position] == ESCAPE_BYTE) {
        stuffedFrame[bcc2Position - 1] = ESCAPE_BYTE;
        stuffedFrame[bcc2Position] = ESCAPED_ESCAPE;
    }
    else {
        stuffedFrame[bcc2Position] = destuffedFrame[bcc2Position];
    }
}

void destuffFrame(char* stuffedFrame, char* destuffedFrame, size_t frameSize) {
    unsigned int currentDestuffedByte = 1;
    unsigned int currentStuffedByte = 1;

    destuffedFrame[0] = FRAMEFLAG;
    destuffedFrame[frameSize - 1] = FRAMEFLAG;

    for(; currentStuffedByte < frameSize - 1; currentStuffedByte++, currentDestuffedByte++) {
        if(stuffedFrame[currentStuffedByte] == ESCAPE_BYTE) {
            currentStuffedByte++;
            destuffedFrame[currentDestuffedByte] = stuffedFrame[currentStuffedByte] ^ XOR_BYTE;
        }
        else
            destuffedFrame[currentDestuffedByte] = stuffedFrame[currentStuffedByte];
    }
}