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
  ApplicationData appdata;
  appdata.s = 0;
  int fd = llopen(argv[1], 0, &appdata);
  if(fd == -1){
    return -1;
  }
  
  /*
  MessageInfo info = readMessage(fd, &appdata);
  if(info.type == ERROR){
    printf("ERror");
    return -1;
  }
  if(info.type == CONTROL && info.data[0] == C_SET){
    writeMessage(fd, A_EMISSOR, C_UA);
  }
  */
 MessageInfo info;
 char *filename;
 long unsigned size = 0;
 FILE* newFD;
 unsigned char nSeq = 0;
  do{
    //wait for file
    info = readMessage(fd, &appdata);
    if(info.type == DATA){
      printf("IT's data");
      //It's control packet
      if(info.data[0] != 1){
        if(info.data[0] == 3){
          fclose(newFD);
          return 1;
        }
        unsigned char i=1;
        printBuffer(info.data, info.nBytes);
        while(i < info.nBytes){
          //printf("--%d\n", i);
          if(info.data[i] == 0){
            unsigned char nChars = info.data[i + 1];
            for(char j=nChars - 1; j >=0; j--){
              size = (size<<8) + info.data[i + 2 + j];
            }
            i+= nChars + 2;
          }
          else if(info.data[i] == 1){
            unsigned char nChars = info.data[i + 1];
            filename = (char *) malloc(1 * nChars);
            for(unsigned char j=0; j < nChars; j++){
              filename[j] = info.data[i + 2 + j];
            }
            i+= nChars + 3;
          }
        }
        printf("Filename = %s\n", filename);
        printf("Filesize = %lu\n", size);
        //Create file
        newFD = fopen(filename, "w+");
        nSeq = 0;
      } else if(info.data[0] == 1){
        //It's file data
        printf("Passou aqui1?\n");
        nSeq = (nSeq + 1) % 255;
        if(nSeq != info.data[1]){
        }
        unsigned numberOfChars = (info.data[1] << 8) + info.data[2];
        for(unsigned i=0; i < numberOfChars; i++){
          fwrite(info.data + 3 + i, 1 , 1, newFD);
        }
        printf("Passou aqui? 2\n");
      }
      
      writeMessage(fd, A_EMISSOR, C_RR + (appdata.s == 1 ? BIT(6): 0));
    } else if(info.type == CONTROL && info.data[0] == C_DISC){
      printf("IT's a disc");
      writeMessage(fd, A_RECETOR, C_DISC);
    }
  } while(!(info.type == CONTROL && info.data[0] == C_DISC));


}
