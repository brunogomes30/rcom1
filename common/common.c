#include "common.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
int isC(char byte){
  return byte == C_SET || byte == C_DISC || byte == C_UA || byte == C_RR || byte == C_REJ;
}

void printBuffer(unsigned char * buffer, unsigned size){
  printf("Buffer = \n");
  for(int i=0; i<  size; i++){
    printf("%x ",buffer[i]);
  }
  printf("\n");
}

int checkBCC(char byte, char *msg){
  return byte == (msg[2] ^ msg[1]);
  
}

int llopen(char *port, int isTransmitter){
  int fd = open(port, O_RDWR | O_NOCTTY);
  printf("fd = %d\n", fd );
  if (fd < 0)
  {
    perror(port);
    //exit(-1);
    return -1;
  }
  struct termios oldtio,newtio;
  if (tcgetattr(fd, &oldtio) == -1)
  { /* save current port settings */
    perror("tcgetattr");
    //exit(-1);
    return -1;
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;
  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 1;  /* blocking read until 5 chars received */

  tcflush(fd, TCIOFLUSH);

  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");

  MessageInfo info;
  info.type = DATA;
  if(isTransmitter){
    printf("Writing message transmitter\n");
    writeMessage(fd, A_EMISSOR, C_SET);  
    printf("Reading message transmitter\n");
    info = readMessage(fd);
    printf("Aqui??\n");
    printf("Message info = %p", &info);
  } else {
    printf("Reading message receiver\n");
    info = readMessage(fd);
    printf("Read message done\n");
    if(info.type != ERROR && info.data[0] == C_SET){
      writeMessage(fd, A_EMISSOR, C_UA);
      
    }
  }
  if(info.type == ERROR){
    printf("ERRO no read message");
    return -1;
  }
 // free(info.data);
  printf("Return llopen\n");
  return fd;
}

int llclose(int fd, int isTransmitter, ApplicationData *appdata){
  //DISC -> DISC -> UA
  if(isTransmitter){
    writeMessage(fd,A_EMISSOR , C_DISC);
    MessageInfo info = readMessage(fd);
    if(info.type == ERROR){

    }
    if(info.type == CONTROL && info.data[0] == C_DISC){
      writeMessage(fd,A_RECETOR ,C_UA);
    }
    close(fd);
  } else {
    MessageInfo info = readMessage(fd);
    if(info.type == ERROR){
      //-1
    }
    if(info.type == CONTROL && info.data[0] == C_DISC){
      writeMessage(fd, A_EMISSOR, C_DISC);
    }
  }
  return 0;
}
int llread(int fd, char *buffer, ApplicationData *appdata){
  return 0;
}

int llwrite(int fd, char *data, int length){
  char buffer[2];
  unsigned nBytes = 0;
  for(int i=0;i<length; i++){
    if(data[i] == FLAG){
      buffer[0] = ESCAPE;
      buffer[1] = FLAG ^ 0x20;
      nBytes +=2;
      write(fd, buffer, 2);
    } else if(data[i] == ESCAPE){
      buffer[0] = ESCAPE;
      buffer[1] = ESCAPE ^ 0x20;
      nBytes += 2;
      write(fd, buffer, 2);
      
    } else {
      write(fd, &data[i], 1);
      nBytes++;
    }
  }
  buffer[0] = data[nBytes-1] ^ data[nBytes - 2];
  write(fd, buffer, 1);
  buffer[0] = FLAG;
  write(fd, buffer, 1);
  printf("Finish writing");
  return 0;
}



State changeState(char byte, State currentState, char *msg){
  printf("State %x: %d",byte,  currentState);
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
  printf(" -> %d\n", currentState);
  return currentState;
}

void writeMessage(int fd, unsigned char address, unsigned char C){
  char msg[5];
  msg[0] = FLAG;
  msg[1] = address;
  msg[2] = C;
  msg[3] = msg[1] ^ msg[2];
  msg[4] =  FLAG;
  //llwrite(fd, msg, 5);
  write(fd, msg, 5);
}

int writeFile(int fd, char *path, ApplicationData *applicationData){
  //Enviar pacote de controlo
  printf("WriteFile\n");
  printf("WriteFile\n");

  struct stat st;
  char *filename;
  stat(filename, &st);
  long filelen = st.st_size;

  printf("Reading file problem???\n");
  printf("Reading file problem???\n");


  
  for(int i=0; path[i] != '\0'; i++){
    if(path[i] == '/') filename = path + 1;
  }
  int filenameSize = strlen(filename);
  char buffer[MAX_SIZE];
  	printf("nice\n");
    printf("nice\n");
  //Prepare control packet
  //C -> T1 L1 V1-> T2 L2 V2
  //O 1 é tamanho
  //O 2 é o nome do ficheiro
  int nBytes = 0;
  buffer[nBytes++] = 0x2; // Start
  buffer[nBytes++] = 0x0; //File length - T
  unsigned char numberOfBytes = filelen * sizeof(char);
  buffer[nBytes++] = numberOfBytes; //Number of bytes - L
  for(unsigned char i=0; i< numberOfBytes; i++){
    buffer[nBytes++] = (filelen >> 8 * i) & 0xff;
  }

  buffer[nBytes++] = 0x1; // Filename
  buffer[nBytes++] = filenameSize * sizeof(char);
  for(char i=0; i< filenameSize; i++){
    buffer[nBytes++] = *(filename + i);
  }
  printf("beforeWirteData\n");
  
  printBuffer(buffer, nBytes);
  writeData(fd, buffer, nBytes, applicationData);
  //llwrite(fd, buffer, nBytes);

  MessageInfo info = readMessage(fd);
  if(info.type == ERROR){
    return -1;
  }
  if(info.s != -1){
    applicationData->s = info.s != applicationData->s ? info.s : applicationData->s;
  }
  

  //Write data
  unsigned PACKET_SIZE = 100; // max  = 65535
  unsigned bytesSent = 0;
  unsigned char seq = 0;
  unsigned char fileData[PACKET_SIZE];
  int fileFD = open(path, "rb");
  while(bytesSent < numberOfBytes){
    nBytes = 0;
    buffer[nBytes++] = 0x1;
    buffer[nBytes++] = seq;
    seq = (seq + 1) % 255;
    int numberOfBytes = filelen;
    
    buffer[nBytes + 1] = numberOfBytes & 0xff;
    buffer[nBytes] = (numberOfBytes >> 8) & 0xff;
    nBytes += 2;
    unsigned bytesSentInThisPacket = 0;
    for(unsigned i=0;i < PACKET_SIZE; i++){
      if(bytesSent == numberOfBytes){
        break;
      }
      read(fileFD, fileData[bytesSentInThisPacket++], 1);
      bytesSent++;
    }
    buffer[nBytes++] = (PACKET_SIZE >> 8) & 0xFF;
    buffer[nBytes++] =  PACKET_SIZE & 0xFF;
    for(unsigned i=0; i< bytesSentInThisPacket; i++){
      buffer[nBytes++] = fileData[i];
    }
    

    writeData(fd, buffer, nBytes, applicationData);
    
    //Read UA
    MessageInfo info = readMessage(fd);
    if(info.type == ERROR){
      return -1;
      //Try again
    }
    if(info.type == CONTROL && info.data[0] == C_UA && info.s != applicationData->s && info.s != -1){
      //good
      applicationData->s = info.s; // Switch S
    } else {
      //Try again
    }
  }
  close(fileFD);
  
  return 0;

}

int writeData(int fd, char *data, int nBytes, ApplicationData *appdata){
  char buffer[nBytes * 2 + 6];
  int i = 0;
  buffer[i++] = FLAG;
  buffer[i++] = A_EMISSOR;
  buffer[i++] = BIT(6) * appdata->s;
  buffer[i++] = A_EMISSOR ^ BIT(6);
  for(int j=0;j<nBytes; j++){
    if(data[j] == FLAG){
      buffer[i++] = ESCAPE;
      buffer[i++] = FLAG ^ 0x20;
    } else if(data[j] == ESCAPE){
      buffer[i++] = ESCAPE;
      buffer[i++] = FLAG ^ 0x20;
    } else {

      buffer[i++] = data[j];
    }
  }
  buffer[i++] = data[nBytes-1] ^ data[nBytes - 2];
  buffer[i++] = FLAG;
  llwrite(fd, buffer, i);
  return 0;
}



MessageInfo readMessage(int fd){
  int i=0;
  char auxBuf[2], msg[5];
  State state = START;
  char data[MAX_SIZE];
  MessageInfo messageInfo;
  messageInfo.s = -1;
  int res = 0;
  int nTries = 0;
  while(nTries < 3){
    printf("Inside while %d\n", state);
    res = read(fd, auxBuf, 1);
    if(res == -1 || res == 0){
      messageInfo.type = ERROR;
      break;
    }
    if(res == 0){
      perror("EOF");
    }
    printf("State:\n");
    state = changeState(auxBuf[0], state, msg);
    printf("state = %d sssss\n", state);
    int s = 0;
    if(state == C_RECV){
      if(auxBuf[0] == BIT(7) || auxBuf[0] == 0) {
        //It's data
        printf("Detected Data\n");
        messageInfo.s = auxBuf[0] == BIT(7);
        int nBytes = readData(fd, data, &state);
        s = (auxBuf[0] & BIT(7)) >> 7;
        if(state != STOP_STATE) {
          messageInfo.type = ERROR;
          break;
          //PASSAR COMO ARGUMENTO???????????
        }

        messageInfo.data = (char *) malloc(1 * nBytes);
        messageInfo.data[0] = *data;
      } else {
        messageInfo.type = CONTROL;
        messageInfo.data = (char *) malloc(1);
        messageInfo.data[0] = auxBuf[0];
      }

    }
    if(state == STOP_STATE){
      break;
    }
    msg[i++] = auxBuf[0]; 
  }
  return messageInfo;
}

int readData(int fd, char *data, State *state){
  char auxBuf[2];
  char skipRead = 0;
  int res;
  int nBytes = 0;
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
      if(res == -1){
        printf("error 409");
        return -1;
      }
      if(auxBuf[1] == FLAG){
        return nBytes;
      } else {
        data[nBytes++] = auxBuf[0];
        auxBuf[0] = auxBuf[1];
        skipRead = 1;
      }
    }


  }
  *state = STOP_STATE;
  return 0;
}