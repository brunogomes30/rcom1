/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include "../common/application.h"
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
  readFile(argv[1]);
 


}
