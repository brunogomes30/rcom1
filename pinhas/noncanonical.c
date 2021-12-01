/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG  0x7E
#define A_TRANSMITTER  0x03
#define A_RECEIVER 0x01
#define SET   0x03
#define DISC  0x0B
#define UA    0x07
#define RR    0x05//r
#define REJ   0x01//r
//R = N(r)
//bcc usar xor com a e c

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd, res;
    struct termios oldtio, newtio;
    char buf[255], msg[5];

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
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");


typedef enum {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP} State;
State curr_state = START;

int isC(char byte){
  return byte == SET || byte == DISC || byte == UA || byte == RR || byte == REJ;
}

int checkBCC(char byte){
  return byte == (msg[2] ^ msg[1]);
}

    while (curr_state != STOP) {       /* loop for input */
      res = read(fd,buf,6);
      printf("0x%x\n", *buf);
      printf("%d bytes recieved\n", res);
      switch(curr_state){
        case START:
            if (*buf == FLAG){
                msg[0]= *buf;
                curr_state = FLAG_RCV;
                }
        case FLAG_RCV:
            if (*buf == A_RCV){
                msg[1]= *buf;
                curr_state = A_RCV;
            }
        case A_RCV:
            if (isC(*buf)){
                msg[2]= *buf;
                curr_state = C_RCV;
            }
        case C_RCV:
            if (checkBCC(*buf)){
                msg[3]= *buf;
                curr_state = BCC_OK;
            }
        case BCC_OK:
            if (*buf == FLAG){
                msg[4]= *buf;
                curr_state = STOP;
            }
        case STOP:
            printf("state is stop\n");
}
        msg[0] = FLAG;
        msg[1] = A_RCV;
        msg[2] = UA;
        msg[3] = msg[1] ^ msg[2];
        msg[4] = FLAG;
        res = write(fd,msg,sizeof(msg));   
        printf("%d bytes written\n", res);


      /*here*/
/*
    if ( tcgetattr(fd,&oldtio) == -1) {  save current port settings 
        perror("tcgetattr");
        exit(-1);
    }*/
    }
      /*here*/
    



  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o 
  */



    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
