/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include "../common/application.h"
#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

//MADE BY ME


//states 
volatile int STOP =FALSE;



State currentState;
char msg[5];

int main(int argc, char** argv) {

  if ((argc < 2)) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }
  
  printf("--%s\n", argv[1]);
  char * filePath = argv[2];
  writeFile(filePath, argv[1]); 
  
  //close(fd);
  return 0;
}
