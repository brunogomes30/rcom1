#include "common.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
int isC(unsigned char byte, char S){
  return byte == C_SET || byte == C_DISC || byte == C_UA || byte == C_RR(S) || byte == C_REJ(S) || byte == (S == 1 ? BIT(6) : 0);
}

void printBuffer(unsigned char * buffer, unsigned size){
  printf("Buffer = \n");
  for(int i=0; i<  size; i++){
    printf("%x ",buffer[i]);
  }
  printf("\n");
}

int checkBCC(unsigned char byte, unsigned char *msg){
  return byte == (msg[2] ^ msg[1]);
  
}
int llopen(char *port, int isTransmitter, ApplicationData *appdata){
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
  //appdata.s = 0;
  if(isTransmitter){
    printf("Writing message transmitter\n");
    writeMessage(fd, A_EMISSOR, C_SET);  
    printf("Reading message transmitter\n");
    info = readMessage(fd, appdata);
    printf("Aqui??\n");
    printf("Message info = %p", &info);
  } else {
    printf("Reading message receiver\n");
    info = readMessage(fd, appdata);
    printf("Read message done\n");
    
    printf("info.data[0] = %x ", info.type);
    if(info.type == CONTROL && info.data[0] == C_SET){
      printf("Sending UA\n");
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
    MessageInfo info = readMessage(fd, appdata);
    if(info.type == ERROR){

    }
    if(info.type == CONTROL && info.data[0] == C_DISC){
      writeMessage(fd,A_RECETOR ,C_UA);
    }
    close(fd);
  } else {
    MessageInfo info = readMessage(fd, appdata);
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

int llwrite(int fd, unsigned char *data, int length){
  unsigned char buffer[2];
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
      write(fd, data + i, 1);
      nBytes++;
    }
  }
  buffer[0] = data[nBytes-1] ^ data[nBytes - 2];
  write(fd, buffer, 1);
  printf("Finish writing \n");
  return 0;
}



State changeState(unsigned char byte, State currentState, unsigned char *msg, char S){
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
      printf("S = %d %x\n", S, byte);
      if(isC(byte, S)){
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

void writeMessage(int fd, unsigned char address, unsigned char C){
  char msg[5];
  msg[0] = FLAG;
  msg[1] = address;
  msg[2] = C;
  msg[3] = msg[1] ^ msg[2];
  msg[4] =  FLAG;
  //llwrite(fd, msg, 5);
  printf("---Write %x \n", C);
  write(fd, msg, 5);
}

int writeFile(int fd, char *path, ApplicationData *applicationData){
  //Enviar pacote de controlo

  struct stat st;
  char *filename;

  filename = path;
  for(int i=0; path[i] != '\0'; i++){
    if(path[i] == '/') filename = path + 1 + i;
  }
  int filenameSize = strlen(filename);
  unsigned char buffer[MAX_SIZE];
  stat(path, &st);
  printf("path = %s\n", path);
  long unsigned  filelen = st.st_size;
  //Prepare control packet
  //C -> T1 L1 V1-> T2 L2 V2
  //O 1 é tamanho
  //O 2 é o nome do ficheiro
  int nBytes = 0;
  buffer[nBytes++] = 0x2; // Start
  buffer[nBytes++] = 0x0; //File length - T
  printf("file len = %lu\n", filelen);


  //for now
  unsigned char numberOfBytes = 4;
  buffer[nBytes++] = numberOfBytes; //Number of bytes - L
  for(unsigned char i=0; i< numberOfBytes; i++){
    buffer[nBytes++] = (filelen >> (8 * i)) & 0xff;
  }

  buffer[nBytes++] = 0x1; // Filename
  buffer[nBytes++] = filenameSize * sizeof(char);
  for(char i=0; i< filenameSize; i++){
    buffer[nBytes++] = *(filename + i);
  }
  printf("beforeWirteData\n");
  
  writeData(fd, buffer, nBytes, applicationData);
  MessageInfo info = readMessage(fd, applicationData);
  if(info.type == ERROR){
    return -1;
  }
  
  applicationData->s = info.s;
  

  //Write data
  unsigned PACKET_SIZE = 100; // max  = 65535
  unsigned long bytesSent = 0;
  unsigned char seq = 0;
  unsigned char fileData[PACKET_SIZE];
  
  FILE* fileFD = fopen(path, "rb");
  while(bytesSent < filelen){
    nBytes = 0;
    buffer[nBytes++] = 0x1;
    buffer[nBytes++] = seq;
    seq = (seq + 1) % 255;
    unsigned bytesSentInThisPacket;
    if(bytesSent + MAX_PER_PACKET > filelen){
      bytesSentInThisPacket = filelen - bytesSent;
    } else {
      bytesSentInThisPacket = MAX_PER_PACKET;
    }
    printf("Byte in this packet = %u\n", bytesSentInThisPacket);
    buffer[nBytes + 1] = bytesSentInThisPacket & 0xff;
    buffer[nBytes] = (bytesSentInThisPacket >> 8) & 0xff;
    nBytes += 2;
    
    for(unsigned i=0;i < bytesSentInThisPacket; i++){
      if(bytesSent == numberOfBytes){
        break; // Tirar depois probably
      }
      fread(fileData + i, 1, 1, fileFD);
      bytesSent++;
    }
    for(unsigned i=0; i< bytesSentInThisPacket; i++){
      buffer[nBytes++] = fileData[i];
    }
    
    writeData(fd, buffer, nBytes, applicationData);
    printf("Sent data packet\n");
    
    //Read UA
    printf("not expecting S = %d", applicationData->s);
    MessageInfo info = readMessage(fd, applicationData);
    if(info.type == ERROR){
      printf("Error detected??\n");
      return -1;
      //Try again
    }
    if(info.type == CONTROL && info.data[0] == C_RR(applicationData->s) && info.s != applicationData->s && info.s != -1){
      //good
      applicationData->s = info.s; // Switch S
    } else {
      printf("Message not rr type = %d, data = %x, s = %d -> appData.s = %d\n", info.type, info.data[0], info.s, applicationData->s);
      //Try again
    }
    printf("bytes Sent = %lu/%lu\n", bytesSent, filelen);
  }
  printf("Saiu do while\n");
  buffer[0] = 3;
  writeData(fd, buffer, 1, applicationData);
  fclose(fileFD);
  
  return 0;

}

int writeData(int fd, unsigned char *data, int nBytes, ApplicationData *appdata){
  printf("Send data S=%d\n", appdata->s);
  unsigned char buffer[nBytes * 2 + 6];
  int i = 0;
  buffer[i++] = FLAG; // Flag
  buffer[i++] = A_EMISSOR; // A
  buffer[i++] = appdata->s == 1 ? BIT(6) : 0; // C 
  buffer[i++] = buffer[1] ^ buffer[2]; // BCC
  //Data begin
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
  //Data end
  buffer[i++] = data[nBytes-1] ^ data[nBytes - 2]; //Last BCC
  
  buffer[i++] = FLAG; // FLag
  printf("Buffer trama = ");
  printBuffer(buffer, i);
  write(fd, buffer, 4);
  llwrite(fd, (buffer + 4), (i - 1 - 4));
  write(fd, buffer + (i - 1), 1);
  return 0;
}



MessageInfo readMessage(int fd, ApplicationData *appdata){
  char expectedS = appdata->s != 1;
  int i=0;
  unsigned char auxBuf[2], msg[5];
  State state = START;
  unsigned char data[MAX_SIZE];
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
    state = changeState(auxBuf[0], state, msg, expectedS);
    if(state == BCC_OK){
      if(msg[2] == BIT(6) || msg[2] == 0) {
        //It's data
        printf("Detected Data\n");
        messageInfo.s = msg[1] == BIT(6);
        int nBytes = readData(fd, data, &state);
        printf("nBytes = %d\n", nBytes);
        if(state != STOP_STATE) {
          messageInfo.type = ERROR;
          break;
          //PASSAR COMO ARGUMENTO???????????
        }
        messageInfo.type = DATA;
        messageInfo.data = (unsigned char *) malloc(1 * nBytes);
        messageInfo.nBytes = nBytes;
        for(int j=0; j<nBytes; j++){
          messageInfo.data[j] = data[j];
        }
      } else {
        messageInfo.type = CONTROL;
        messageInfo.data = (unsigned char *) malloc(1);
        messageInfo.nBytes = 1;
        messageInfo.data[0] = msg[i - 1];
        messageInfo.s = (messageInfo.data[0] & BIT(8)) != BIT(8);
        if(msg[i - 1] == C_SET || msg[i - 1] == C_DISC || msg[i - 1] == C_UA){

        } else {
          //appdata->s = messageInfo.s;
        }
      }

    }
    if(state == STOP_STATE){
      break;
    }
    msg[i++] = auxBuf[0]; 
    
  }
  return messageInfo;
}

int readData(int fd, unsigned char *data, State *state){
  char auxBuf[2];
  char skipRead = 0;
  int res;
  int nBytes = 0;
  while(auxBuf[0] != FLAG){
    if(!skipRead){
      res = read(fd, auxBuf, 1);
    }
    
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
        *state = STOP_STATE;
        return nBytes;
      } else {
        data[nBytes++] = auxBuf[0];
        auxBuf[0] = auxBuf[1];
        skipRead = 1;
      }
    } else {
      data[nBytes++] = auxBuf[0];
    }
  

  }
  *state = STOP_STATE;
  return 0;
}