#include "datalink.h"

unsigned char S = 0;

//Fazer application.h e application.c

int writeData(int fd, unsigned char *data, unsigned nBytes){
    unsigned char dataFrame[MAX_SIZE];
    unsigned dataFrameSize = buildDataFrame(data, nBytes, dataFrame);
    unsigned tries = 0;
    unsigned char exitWhile = 0;
    while(tries < 5){
        write(fd, dataFrame, nBytes);

        MessageInfo info = readMessage(fd);
        switch(info.type){
            case ERROR:
                tries++;
                break;
            case CONTROL:
                if(info.data[0] == C_RR(S != 1)){
                    S = info.s;
                    exitWhile = 1;
                }
                break;
            case DATA:
                printf("Leu data?? WTF???\n");
                break;
        }
        if(exitWhile) 
            break;
    }

    if(tries == 5){
        printf("Failed to write 5 times in a row\n");
        return -1;
    }
    return 0;
}

unsigned buildDataFrame(unsigned char* data, unsigned nBytes, unsigned char *dataFrame){
    unsigned char buffer[MAX_SIZE];
    unsigned bufferSize = 0;
    unsigned dataFrameSize = 4;
    buffer[0] = FLAG;
    buffer[1] = A_EMISSOR;
    buffer[2] = S == 1 ? BIT(6) : 0;
    buffer[3] = buffer[1] ^ buffer[2];

    unsigned stuffedDataSize = stuffData(data, nBytes, buffer + dataFrameSize);
    bufferSize = dataFrameSize + stuffedDataSize;

    //Build last BCC
    unsigned char bcc = 0;
    for(unsigned i=0; i<nBytes; i++){
        bcc = bcc ^data[i];
    }
    
    buffer[bufferSize++] = bcc;
    buffer[bufferSize++] = FLAG; 
    return bufferSize;   
}

unsigned stuffData(unsigned char *data, unsigned nBytes, unsigned char *stuffedData){
    unsigned stuffedSize = 0;
    for(int i=0;i<nBytes; i++){
        if(data[i] == FLAG){
            stuffedData[0] = ESCAPE;
            stuffedData[1] = FLAG ^ 0x20;
            stuffedSize +=2;
            write(fd, buffer, 2);
        } else if(data[i] == ESCAPE){
            stuffedData[0] = ESCAPE;
            stuffedData[1] = ESCAPE ^ 0x20;
            stuffedSize += 2;
            write(fd, buffer, 2);
        } else {
            write(fd, stuffedData + i, 1);
            stuffedSize++;
        }
  }
  return stuffedSize;
}

void writeMessage(int fd, unsigned char address, unsigned char C){
    unsigned char buffer[5];
    buffer[0] = FLAG;
    buffer[1] = address;
    buffer[2] = C;
    buffer[3] = address ^ C;
    buffer[4] = FLAG;
    write(fd, buffer, 5);
}

MessageInfo readMessage(int fd){
    unsigned char buffer[MAX_SIZE];
    unsigned bufferSize = 0;
    State state = START;
    unsigned isData = 0;
    unsigned stuffedDataSize = 0;
    MessageInfo info;
    while(state != STOP_STATE){
        int res = read(fd, buffer + bufferSize, 1);
        //Verificar timeout com o res
        State previousState;
        state = changeState(buffer[bufferSize], state, buffer, S);
        if(state == FLAG_RECV){
            buffer[0] = FLAG;
            bufferSize = 1;
            stuffedDataSize = 0;
        } else if(state == START){
            bufferSize = 0;
        }

        if(state == C_RECV){
            unsigned isData = buffer[bufferSize] == (S == 1 ? BIT(6) : 0)
            info.type = isData ? DATA : CONTROL;
        }
        if(state == BCC_OK){
            if(info.type == DATA){
                stuffedDataSize++;
            }
            else if(info.type == CONTROL){
                info.data = (unsigned char *) malloc(1 * sizeof(unsigned char));
                info.dataSize = 0;
                info.data[0] = buffer[2];
            }
        }   
    }
    if(state != STOP_STATE){
        info.type = ERROR;
        info.dataSize = 0;
        info.s = S;
        return info;
    }

    unsigned char stuffedData[stuffedDataSize];
    stuffedDataSize -= 2;
    memccpy(stuffedData, buffer + 4, stuffedDataSize);
    unsigned char data[stuffedDataSize];
    unsigned dataSize = unstuffData(stuffedData, data, stuffedDataSize, buffer[stuffedDataSize + 4 + 1]);
    if(dataSize == -1){
        //Try again
        info.type = ERROR;
        info.s = S;
        info.dataSize = 0;
        return info;
    }
    info.data = (unsigned char *) malloc(sizeof(sizeof(unsigned char) * dataSize));
    info.dataSize = dataSize;
    info.s = S != 1;
    return info;
 }

 unsigned unstuffData(unsigned char * stuffedData,unsigned char *data, unsigned stuffedDataSize){
     unsigned bcc = 0;
     unsigned dataSize = 0;
     unsigned previousIsEscape = 0;
     //Verificar se stuffedData não é flag
     for(unsigned i=0; i < stuffedDataSize; i++){
         unsigned char c = stuffedData[i];
         if(previousIsEscape){
             previousIsEscape = 0;
             if(c == 0x5e){
                 c = FLAG;
             } else if(c == 0x5d){
                 c = ESCAPE;
             } else {
                 printf("Mandou escape sem flag ou escape############################\n");
             }
         }
         else if(c == ESCAPE){
             previousIsEscape = 1;
         }  else if(c == FLAG){
             printf("Mandou flag na stuffedData#################################################\n");
         } else if(!previousIsEscape){
             data[dataSize++] = c;
             bcc = bcc ^ c;
         }
     }

     if(bcc != givenBCC) return -1;
     return dataSize;
 }