/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "supervisionFrame.h"

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;
    
    if ( (argc < 2) ||
        ((strcmp("/dev/ttyS0", argv[1])!=0) &&
         (strcmp("/dev/ttyS1", argv[1])!=0) )) {
            printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
            exit(1);
        }
    
    
    /*
     Open serial port device for reading and writing and not as controlling tty
     because we don't want to get killed if linenoise sends CTRL-C.
     */
    
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }
    
    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
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
        exit(-1);
    }
    
    printf("New termios structure set\n");
    
    
    // printf("message:\n");
    // gets(buf);
    // int sizeLine = strlen(buf)+1;
    
    // res = write(fd,buf,sizeLine);
    
    SupervisionFrame frame = createFrame(RECEIVER_ADDRESS, SET);
    
    printf("%x\n", frame.frameHeader);
    printf("%x\n", frame.address);
    printf("%x\n", frame.control);
    printf("%x\n", frame.bcc);
    printf("%x\n", frame.frameTrailer);    
    
    struct sigaction sa;
    sa.sa_handler = timeout;
    sa.sa_flags = 0;
    
    sigaction(SIGALRM, &sa, NULL);
    
    int continueTrying = 1;
    SupervisionFrame receivedFrame;
    
    char finalstring[255];
        
    while (retryCounter < 4) {

        if (retryCounter > 0) printf("Retry #%d\n", retryCounter);
        int curchar = 0;
        res = write(fd, &frame, sizeof(SupervisionFrame));
        printf("%d bytes written\n", res);
        alarm(3);
        int currentTry = retryCounter;

        while ( STOP == FALSE && retryCounter == currentTry ) {
            res = read(fd,buf,1);
            if (res == 1) {
                finalstring[curchar] = buf[0];
                curchar++;
            }
            if (finalstring[curchar-1] == FRAMEFLAG && curchar-1 > 0) STOP = TRUE;
        }

        if ( STOP == TRUE ) {
            memcpy(&receivedFrame, finalstring, sizeof(SupervisionFrame));
            if ( receivedFrame.address ^ receivedFrame.control == receivedFrame.bcc ){
                break;
            }
            STOP == FALSE;
        }
    }
    
    
    
    if (STOP == TRUE) {
        
        //res = write(fd, &frame, sizeof(SupervisionFrame));
        
        printf("%x\n", receivedFrame.frameHeader);
        printf("%x\n", receivedFrame.address);
        printf("%x\n", receivedFrame.control);
        printf("%x\n", receivedFrame.bcc);
        printf("%x\n", receivedFrame.frameTrailer);
    }
    else
        printf("Failed!\n");
    
    /*
     int curchar = 0;
     char finalstring[255];
     
     while ( STOP == FALSE ) {
     res = read(fd,buf,1);
     if (res == 1) {
     finalstring[curchar] = buf[0];
     curchar++;
     }
     if (finalstring[curchar-1] == '\0') STOP = TRUE;
     
     
     }
     
     printf("received: %s\n", finalstring);
     */
    
    /*
     O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
     o indicado no guião 
     */
    
    
    
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    
    
    
    
    close(fd);
    return 0;
}
