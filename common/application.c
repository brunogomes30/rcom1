#include "application.h"
#include "datalink.h"
#include <time.h>

int writeFile(char *file, char *port){
    printf("Establishing connection with receiver...\n");
    int fd = llopen(port, 1);
    if(fd == -1){
        printf("Couldn't established connection with receiver\n");
        return -1;
    }
    printf("Established connection with receiver\n");
    printf("Writing control Packet...\n");
    FileData fileData = readFileData(file);
    
    //Write start packet
    writeControlPacket(fileData, fd, 1);
    printf("Wrote control packet successfully\n");

    unsigned long filelen = fileData.filelen;
    FILE* fileFD = fopen(file, "rb");
    unsigned char buffer[MAX_SIZE];
    unsigned char dataPacket[MAX_SIZE];
    unsigned long bytesWritten = 0;
    unsigned char nseq = 0;
    unsigned failedToWrite = 0;
    while(bytesWritten < filelen){
        nseq = (nseq + 1) % 255;
        printf("Bytes written so far: %lu\n", bytesWritten);
        unsigned long percentage = (bytesWritten * 100) / filelen;
        printf("%lu%% Done\n", percentage);
        unsigned int readInThisPacket = filelen - bytesWritten < MAX_PER_PACKET ? filelen - bytesWritten : MAX_PER_PACKET;
        //buffer[2] = 
        fread(buffer, 1, readInThisPacket, fileFD);
        bytesWritten += readInThisPacket;
        //Send packet
        int res, ntries = 0;
        unsigned int dataPacketSize = assembleDataPacket(dataPacket, buffer, readInThisPacket, nseq);
        do{
            ntries ++;
            printf("Writting %d bytes... try nº%d\n", readInThisPacket, ntries);
            res = llwrite(fd, dataPacket, dataPacketSize);
            if(res == -1){
                printf("#### trying again\n");
                continue;
            } else {
                break;
            }
        } while(ntries < 5);
        if(ntries == 5){
            failedToWrite = 1;
            break;
        }
        printf("proceeding to next packet\n");
    }
    if(failedToWrite){
        printf("Failed to write file \n");
        return -1;
    }
    //Write end packet
    writeControlPacket(fileData, fd, 0);
    printf("Sent file successfully, closing...\n");
    llclose(fd, 1);
    return 0;
}
FileData readFileData(char *file){
    FileData fileData;
    struct stat st;
    char *filename;
    filename = file;
    for(int i=0; file[i] != '\0'; i++){
        if(file[i] == '/' ) filename = file + 1 + i;
    }
    fileData.filenameLen = strlen(filename);
    stat(file, &st);
    fileData.filenameLen = strlen(filename);
    fileData.filename = (char *) malloc(strlen(filename) * 1 + 1);
    memcpy(fileData.filename, filename,fileData.filenameLen);
    fileData.filelen = st.st_size;
    return fileData;
}

int writeControlPacket(FileData fileData, int fd, int isStart){
    printf("filename = %s size = %lu \n", fileData.filename, fileData.filelen);

    unsigned char buffer[MAX_SIZE];
    unsigned nBytes = 0;
    buffer[nBytes++] = isStart == 1 ? 2 : 3;
    buffer[nBytes++] = 0;
    buffer[nBytes++] = 4;
    memcpy(buffer+ nBytes, &fileData.filelen, 4);
    nBytes += 4;
    buffer[nBytes++] = 1;
    buffer[nBytes++] = fileData.filenameLen + 1;
    for(int i=0; i < fileData.filenameLen; i++){
        buffer[nBytes++] = fileData.filename[i];
    }
    buffer[nBytes++] = 0;
    writeData(fd, buffer, nBytes);
    return 1;
}

unsigned int assembleDataPacket(unsigned char *dataPacket , unsigned char* data, unsigned int size, unsigned char nseq){
    dataPacket[0] = 1;
    dataPacket[1] = nseq;
    memcpy(dataPacket + 2, &size, 2);
    memcpy(dataPacket + 4, data, size);
    return size + 4;
}

int readFile(char * port){
    
    printf("Establishing connection with transmiter...\n");
    int fd = llopen(port, 0);
    if(fd == -1){
        printf("Couldn't establish connection with transmitter\n");
        return -1;
    }
    printf("Established connection with transmitter\n");

    //unsigned char buffer[MAX_SIZE];
    //unsigned long filelen = 0;
    unsigned finished = 0;
    FileData fileDataStart, fileDataEnd;
    FILE* fileFD;
    struct timespec tbegin={0,0}, tend={0,0};
    unsigned char data[MAX_SIZE];
    int dataSize = 0;
    clock_gettime(CLOCK_MONOTONIC, &tbegin);
    unsigned long bytesReadSoFar = 0;
    unsigned currentNSeq = 1;
    unsigned ntries = 0;
    while(!finished && ntries < 5){
        
       dataSize = llread(fd, data);
       if(dataSize != -1){
           ntries = 0;
            switch(data[0]){
                case 1:
                    if(currentNSeq == data[1]){
                        currentNSeq = (currentNSeq + 1 ) % 255;
                        writeToFile(data + 4, dataSize - 4, fileDataStart);
                        bytesReadSoFar += dataSize - 4;
                        printf("%lu%% done\n", (bytesReadSoFar * 100) / fileDataStart.filelen);
                    }
                    break;
                case 2:
                    //Control packet begin
                    fileDataStart = readControlPacket(data, dataSize);
                    fileFD = fopen((char *) fileDataStart.filename, "w+");
                    printf("Receiving file %s of size %lu Bytes...\n", fileDataStart.filename, fileDataStart.filelen);
                    fclose(fileFD);
                    break;
                case 3:
                    //Control packet end
                    fileDataEnd = readControlPacket(data, dataSize);
                    finished = 1;
                    break;
            }
        } else {
            ntries++;
        }

    }

    if(ntries == 5){
        printf("Lost connection with transmitter\n");
        return -1;
    }
    if(fileDataEnd.filelen != fileDataStart.filelen){
        printf("error control packet end and control packet begin don't match\n");
        return -1;
    }
    printf("Received file %s of size %lu Bytes...\n", fileDataStart.filename, fileDataStart.filelen);
    clock_gettime(CLOCK_MONOTONIC, &tend);
    printf("Closing port...\n");
    llclose(fd, 0);
    printf("Successfully closed port\n");

    return 0;
}


FileData readControlPacket(unsigned char *packet, unsigned int nBytes){
    FileData fileData;
    unsigned currByte = 0;
    unsigned int type = 0;
    unsigned int typeSize = 0; 


    if (packet[currByte] == 2){
        //printf("Reading start control packet");
    }
    else if (packet[currByte] == 3){
        //printf("Reading end control packet");
    }
    else {
        perror("Wrong C in control packet");
    }
    currByte++;
    fileData.filelen = 0;
    while (currByte < nBytes){
        type = packet[currByte++];
        typeSize = packet[currByte++];
        
        if (type == 0){
            memcpy(&fileData.filelen, packet + currByte, 4);
            currByte += typeSize;
        }
        else if (type == 1){
            fileData.filenameLen = typeSize;
            fileData.filename = (char*) malloc(typeSize * 1);
            memcpy(fileData.filename, packet + currByte, typeSize);
            currByte += typeSize;
        }
        else {
            perror("Undefined type for control packet");
        }
    } 
    return fileData;
}

int writeToFile(unsigned char* data, unsigned int dataSize, FileData fileData){
    
    FILE* fileFD = fopen(fileData.filename, "ab");
    fwrite(data, 1, dataSize, fileFD);
    fclose(fileFD);
    
    return 0;
}



