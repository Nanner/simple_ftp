#include "linkLayerProtocol.h"

int llopen(int port, int role){

    int fd, res;
    struct termios oldtio,newtio;
    char buf[255];
    int STOP=FALSE;
    /*
     Open serial port device for reading and writing and not as controlling tty
     because we don't want to get killed if linenoise sends CTRL-C.
     */

    if ( port == COM1 ) {
    	fd = open(COM1_PORT, O_RDWR | O_NOCTTY );
    } else if ( port == COM2 ) {
    	fd = open(COM2_PORT, O_RDWR | O_NOCTTY );
    } else fd = -1;

    if (fd < 0) {
        perror("port error"); exit(-1); 
    }
    
    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
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
    
    tcflush(fd, TCIOFLUSH);
    
    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
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
        SupervisionFrame frame = createFrame(RECEIVER_ADDRESS, SET);
        SupervisionFrame receivedFrame;
        
        char receivedString[255];
        
        retryCounter = 0;
            
        while (retryCounter < 4) {

            if (retryCounter > 0) printf("Retry #%d\n", retryCounter);
            int curchar = 0;
            res = write(fd, &frame, sizeof(SupervisionFrame));
            printf("%d bytes sent\n", res);
            alarm(3);
            int currentTry = retryCounter;

            while ( STOP == FALSE && retryCounter == currentTry ) {
                res = read(fd,buf,1);
                if (res == 1) {
                    receivedString[curchar] = buf[0];
                    curchar++;
                }
                if (receivedString[curchar-1] == FRAMEFLAG && curchar-1 > 0) STOP = TRUE;
            }

            if ( STOP == TRUE ) {
                memcpy(&receivedFrame, receivedString, sizeof(SupervisionFrame));
                if ( checkBcc(receivedFrame) ){
                    if (receivedFrame.control == UA ) {
                        printf("Handshake sucess!\n");
                    } else {
                        printf("Didn't receive UA!\n");
                    }
                    alarm(0);
                    break;
                }
                STOP = FALSE;
            }
        }
    }

    if (role == RECEIVER)
    {
        printf("Waiting for connection...");
        int curchar = 0;
        char receivedString[255];
        
        while (STOP==FALSE) {
            res = read(fd,buf,1);
            if(res == 1){
                receivedString[curchar] = buf[0];
                curchar++;
            }
            if (receivedString[curchar-1]==FRAMEFLAG && curchar-1 > 0) STOP=TRUE;
        }
        
        SupervisionFrame frame;
        memcpy(&frame, receivedString, sizeof(SupervisionFrame));
        printf("%x %x %x %x %x\n", frame.frameHeader, frame.address, frame.control, frame.bcc, frame.frameTrailer);
        
        SupervisionFrame confirmationFrame = createFrame(SENDER_ADDRESS, UA);
        
        res=write(fd, &confirmationFrame, sizeof(SupervisionFrame));
        printf("\nsent\n");
    }


    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    
    return fd;
}

int llclose(int fd) {
    
    
    return 0;
}