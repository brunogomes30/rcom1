/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include "../common/common.h"
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */

//R = N(r)
//bcc usar xor com a e c

volatile int STOP = 0;

int main(int argc, char **argv)
{
  

  if ((argc < 2))
  {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  int fd = llopen(argv[1], 0);
  if(fd == -1){
    return -1;
  }
  printf("Ready to receive message");
  MessageInfo info = readMessage(fd);
  if(info.type == ERROR){
    printf("ERror");
    return -1;
  }
  if(info.type == CONTROL && info.data[0] == C_SET){
    printf("C_SET RECEIVED, writing C_UA");
    writeMessage(fd, A_EMISSOR, C_UA);
  }

  while(!(info.type == CONTROL && info.data[0] == C_DISC)){
    //wait for file
    printf("waiting for something\n");
    info = readMessage(fd);
    if(info.type == DATA){
      //It's control packet
      printf("IT's data");

      //It's file data

    } else if(info.type == CONTROL && info.data[0] == C_DISC){
      printf("IT's a disc");
      writeMessage(fd, A_RECETOR, C_DISC);
    }
  }


}
