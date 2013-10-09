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
    }

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
    
    printf("New termios structure set\n");
        
    // install timeout handler
    struct sigaction sa;
    sa.sa_handler = timeout;
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    if (role == TRANSMITTER) 
    {
        Frame frame = createSupervisionFrame(RECEIVER_ADDRESS, SET);
        Frame receivedFrame;
        
        char receivedString[255];
            
        while (retryCounter < 4) {

            if (retryCounter > 0) printf("Retry #%d\n", retryCounter);
            int curchar = 0;
            res = write(fd, &frame, sizeof(Frame));
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
                memcpy(&receivedFrame, receivedString, sizeof(Frame));
                if ( (receivedFrame.address ^ receivedFrame.control) == receivedFrame.bcc1 ){
                    if (receivedFrame.control == UA ) {
                        printf("Handshake sucess!\n");
                    } else {
                        printf("Didn't receive UA!\n");
                    }
                    break;
                }
                STOP = FALSE;
            }
        }
        
        
        
        if (STOP == TRUE) {
            
            //res = write(fd, &frame, sizeof(SupervisionFrame));
            
            printf("%x\n", receivedFrame.frameHeader);
            printf("%x\n", receivedFrame.address);
            printf("%x\n", receivedFrame.control);
            printf("%x\n", receivedFrame.bcc1);
            printf("%x\n", receivedFrame.frameTrailer);
        }
        else
            printf("Failed!\n");
    }

    if (role == RECEIVER)
    {
        int curchar = 0;
        char finalstring[255];
        
        while (STOP==FALSE) {
            res = read(fd,buf,1);
            if(res == 1){
                finalstring[curchar] = buf[0];
                curchar++;
            }
            if (finalstring[curchar-1]==FRAMEFLAG && curchar-1 > 0) STOP=TRUE;
        }
        
        Frame frame;
        memcpy(&frame, finalstring, sizeof(Frame));
        printf("%x %x %x %x %x\n", frame.frameHeader, frame.address, frame.control, frame.bcc1, frame.frameTrailer);
        
        Frame confirmationFrame = createSupervisionFrame(SENDER_ADDRESS, UA);
        
        res=write(fd, &confirmationFrame, sizeof(Frame));
        printf("\nsent\n");
    }


    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    
    return fd;
}