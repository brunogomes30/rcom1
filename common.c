#include "common.h"

int isC(char byte){
  return byte == C_SET || byte == C_DISC || byte == C_UA || byte == C_RR || byte == C_REJ;
}

int checkBCC(char byte, char *msg){
  return byte == msg[2] ^ msg[1];
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