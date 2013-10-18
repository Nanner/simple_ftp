#include "linkLayerProtocol.h"

int llopen(int port, int role){

    struct termios oldtio,newtio;
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
    
    if ( tcgetattr(applicationLayerConf.fileDescriptor,&oldtio) == -1) { /* save current port settings */
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

    // install timeout handler
    struct sigaction sa;
    sa.sa_handler = timeout;
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    if (role == TRANSMITTER) 
    {
        int STOP=FALSE;
        Frame frame = createSupervisionFrame(RECEIVER_ADDRESS, SET);
        Frame receivedFrame;
        
        retryCounter = 0;
            
        while (retryCounter < 4) {

            if (retryCounter > 0)
                printf("Retry #%d\n", retryCounter);
            printf("Sending: %x %x %x %x %x\n", frame.frameHeader, frame.address, frame.control, frame.bcc1, frame.frameTrailer);
            int res = toPhysical(&frame);
            printf("%d bytes sent\n", res);

            alarm(3);
            int currentTry = retryCounter;

            while (STOP == FALSE && retryCounter == currentTry) {
                printf("Waiting for UA\n");
                res = fromPhysical(&receivedFrame, 1);

                if(res != -1) {
                    if (validBCC1(receivedFrame)){
                        if (receivedFrame.control == UA ) {
                            printf("Handshake sucess!\n");
                            alarm(0);
                            STOP = TRUE;
                        } else {
                            printf("Didn't receive UA!\n");
                        }
                    }
                    else {
                        printf("BCC1 check failed!\n");
                    }
                }
            }

            if(STOP == TRUE)
                break;
        }
        
        if (STOP == TRUE) {
            printf("\nReceived:\n%x %x %x %x %x\n", receivedFrame.frameHeader, receivedFrame.address, receivedFrame.control, receivedFrame.bcc1, receivedFrame.frameTrailer);
        }
        else
            printf("Failed!\n");
    }

    if (role == RECEIVER)
    {
        printf("Waiting for connection...\n");
        
        Frame receivedFrame;
        fromPhysical(&receivedFrame, 0);
        printf("\nReceived:\n%x %x %x %x %x\n", receivedFrame.frameHeader, receivedFrame.address, receivedFrame.control, receivedFrame.bcc1, receivedFrame.frameTrailer);
        
        Frame confirmationFrame = createSupervisionFrame(SENDER_ADDRESS, UA);
        int res = toPhysical(&confirmationFrame);
        printf("Sending %d bytes: %x %x %x %x %x\n", res, confirmationFrame.frameHeader, confirmationFrame.address, confirmationFrame.control, confirmationFrame.bcc1, confirmationFrame.frameTrailer);
    }


    if ( tcsetattr(applicationLayerConf.fileDescriptor,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    
    return applicationLayerConf.fileDescriptor;
}

int llclose(int fd) {
    
    
    return 0;
}

void timeout() {
    retryCounter++;
}

int toPhysical(Frame* frame) {
    tcflush(applicationLayerConf.fileDescriptor, TCIOFLUSH);
    return write(applicationLayerConf.fileDescriptor, frame, sizeof(Frame));
}

int fromPhysical(Frame* frame, int exitOnTimeout) {
    int curchar = 0;
    char receivedString[sizeof(Frame)];
    char buf[sizeof(Frame)];
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
        if (receivedString[curchar-1]==FRAMEFLAG && curchar-1 > 0 && curchar == sizeof(Frame)) STOP=TRUE;
        if(exitOnTimeout && (currentTry < retryCounter))
            return -1;
    }

    memcpy(frame, receivedString, sizeof(Frame));
    return curchar;
}