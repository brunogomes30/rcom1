#include "common.h"
#include <stdlib.h>
int isC(char byte){
  return byte == C_SET || byte == C_DISC || byte == C_UA || byte == C_RR || byte == C_REJ;
}

int checkBCC(char byte, char *msg){
  return byte == (msg[2] ^ msg[1]);
  
}
State changeState(char byte, State currentState, char *msg){
  switch(currentState){
    case START:
      if(byte == FLAG)
        currentState = FLAG_RECV;
        msg[0] = byte;
      break;
    case FLAG_RECV:
      if(byte == FLAG){
        currentState = FLAG_RECV;
        msg[0] = byte;
      } else if(byte == A_EMISSOR || byte == A_RECETOR) {
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
      if(checkBCC(byte, msg)){
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
  return currentState;
}


void writeData(int fd, char *data, int nBytes){
  write(fd, FLAG, 1);
  write(fd, A_EMISSOR, 1);
  //write(fd, C_)
  //write(fd, A_EMISSOR ^ , 1);
  for(int i=0;i<nBytes; i++){
    if(data[i] == FLAG){
      write(fd, ESCAPE, 1);
      write(fd, FLAG ^ 0x20);
    } else if(data[i] == ESCAPE){
        write(fd, ESCAPE, 1);
        write(fd, ESCAPE ^ 0x20);
    } else {
      write(fd, data[i], 1);
    }
  }
  write(fd, data[nBytes-1] ^ data[nBytes - 2], 1);
}



int readData(int fd, char* data){
  int nBytes = 0;
  int i=0;
  char auxBuf[2], msg[5];
  int res;
  State state = START;
  while(state != BCC_OK){
    res = read(fd, auxBuf, 1);
    state = changeState(auxBuf[0], state, msg);
    msg[i++] = auxBuf[0]; 
  }
  i=0;
  int skipRead = 0;
  while(auxBuf[0] != FLAG){
    if(!skipRead)
      res = read(fd, auxBuf, 1);
    skipRead = 0;

    if(auxBuf[0] == ESCAPE){
      res = read(fd, auxBuf + 1, 1);
      if(auxBuf[1] == 0x5e){
        data[nBytes++] = FLAG;
      } else if(auxBuf[1] == 0x5d){
        data[nBytes++] = ESCAPE;
      }
      else {
        perror("readdata error");
        return -1;
      }

      data[nBytes++] = auxBuf[0];
      skipRead = 1;
      auxBuf[0] = auxBuf[1];
    } else if(auxBuf[0] == (data[nBytes-1] ^ data[nBytes - 2])){
      res = read(fd, auxBuf + 1, 1);

      if(auxBuf[1] == FLAG){
        return nBytes;
      } else {
        data[nBytes++] = auxBuf[0];
        auxBuf[0] = auxBuf[1];
        skipRead = 1;
      }
    }


  }
}