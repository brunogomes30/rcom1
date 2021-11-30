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
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

//MADE BY ME
#define FLAG  0x7e
#define A_EMISSOR  0x03
#define A_RECETOR  0x01
#define C_SET  0x03
#define C_DISC  0x11
#define C_UA  0x07
#define C_RR  0x05
#define C_REJ  0x01

//states 
volatile int STOP =FALSE;

typedef enum  {START, FLAG_RECV, A_RECV, C_RECV, BCC_OK, STOP_STATE} State;

State currentState;
char msg[5];

int isC(char byte){
  return byte == C_SET || byte == C_DISC || byte == C_UA || byte == C_RR || byte == C_REJ;
}

int checkBCC(char byte){
  return byte == (msg[2] ^ msg[1]);
}
void changeState(char byte){
  switch(currentState){
    case START:
      if(byte == FLAG){
        currentState = FLAG_RECV;
        msg[0] = byte;
      }
      break;
    case FLAG_RECV:
      if(byte == FLAG){
        currentState = FLAG_RECV;
        msg[0] = byte;
      } else if(byte == A_RECV) {
        currentState = A_RECV;
        msg[1] = byte;
      } else {
        currentState = START;
      }
      break;
    case A_RECV:
      if(isC(byte)){
        currentState = C_RECV;
        msg[2] = byte;
      } else if(byte == FLAG){
        msg[0] = byte;
        currentState = FLAG_RECV;
      } else {
        currentState = START;
      }
      break;
    case C_RECV:
      if(checkBCC(byte)){
        msg[3] = byte;
        currentState = BCC_OK;
      } else if(byte == FLAG){
        msg[0] = byte;
        currentState = FLAG_RECV;
      } else {
        currentState = START;
      }
      break;
    case BCC_OK:
      if(byte == FLAG){
        msg[4] = byte;
        currentState = STOP_STATE;
      } else {
        currentState = START;
      }
      break;
    case STOP_STATE:
      break;
  }
}
int main(int argc, char** argv)
{
    int fd, res;
    struct termios oldtio,newtio;
    //char buf[255];
    //int i, sum = 0, speed = 0;
    
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
    
    

    //DEFINE SET
    msg[0] = FLAG;
    msg[1] = A_EMISSOR;
    msg[2] = C_SET;
    msg[3] = msg[1] ^ msg[2];
    msg[4] =  FLAG;
    res = write(fd,msg,sizeof(msg));   
    printf("%d bytes written\n", res);
 

  /* 
    O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar 
    o indicado no gui�o 
  */



    sleep(2);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }


    currentState = START;
    //READ UA pls
    char byte[255];
    while (STOP==FALSE) {     
      res = read(fd,byte,1);   
      //char byteV = *byte;
      changeState(*byte);
      if(currentState == STOP_STATE){
        printf("Success!");
        if(msg[2] == C_UA){
          printf("C_UA RECEIVED\n");
        }
        
        break;
      }
      //if (byte[0]=='\0') STOP=TRUE;
    }
    



    close(fd);
    return 0;
}
