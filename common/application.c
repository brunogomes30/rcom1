#include "application.h"
#include "datalink.h"

int writeFile(char *file, char *port){
    printf("Openning port...\n");
    int fd = llopen(port, 1;
    if(fd == -1){
        return -1;
    }
    printf("Opened succesfully\n");
    printf("Writing control Packet...\n");
    FileData fileData = readFileData(file);
    
    //Write start packet
    writeControlPacket(fileData, fd, 1);
    printf("Wrote control packet successfully\n");

    unsigned long filelen = fileData.filelen;
    FILE* fileFD = fopen(path, "rb");
    unsigned char buffer[MAX_PER_PACKET];
    unsigned long bytesWritten = 0;
    while(bytesWritten < filelen){
        unsigned char readInThisPacket = filelen - bytesWritten < MAX_PER_PACKET ? filelen - bytesWritten : MAX_PER_PACKET;
        fread(buffer, 1, readInThisPacket, fileFD);
        bytesWritten += readInThisPacket;
        int res, ntries = 0;
        do{
            ntries ++;
            printf("Writting %d bytes... try nº%d", readInThisPacket, ntries);
            res = writeData(fd, buffer, readInThisPacket);
        } while(ntries < 5);
        printf("proceeding to next packet\n");
    }

    //Write end packet
    writeControlPacket(fileData, fd, 0);
    printf("Sent file successfully, closing...\n");
    llclose(fd, 1);
}

FileData readFileData(char *file){
    FileData fileData;
    struct stat st;
    char *filename;
    filename = path;
    for(int i=0; path[i] != '\0'; i++){
        if(path[i] == '/') filename = path + 1 + i;
    }
    
    int filenameSize = strlen(filename);
    
    unsigned char buffer[MAX_SIZE];
    stat(path, &st);
    fileData.filenameLen = strlen(filename);
    fileData.filename = (char *) malloc(strlen(filename) * 1);
    memccpy(fileData.filename, filename, fileData.filenameLen);
    fileData.filelen = st.st_size;
    return fileData;
}

int writeControlPacket(FileData fileData, int fd, int isStart){
    unsigned char buffer[MAX_SIZE];
    unsigned nBytes = 0;
    buffer[nBytes++] = isStart == 1 ? 2 : 3;
    buffer[nBytes++] = 0;
    buffer[nBytes++] = 4;
    for(int i=0; i < 4; i++){
        buffer[nBytes++] = (filelen >> (8 * i)) & 0xff;
    }
    buffer[nBytes++] = 1;
    buffer[nBytes++] = fileData.filenameLen;
    for(int i=0; i < fileData.filenameLen; i++){
        buffer[nBytes++] = fileData.filename[i];
    }
    return 1;
}

int readFile(char *file){
    printf("openning port\n");
    int fd = llopen(port, 0);
    printf("opened successfully\n");

    unsigned char buffer[MAX_SIZE];
    unsigned long filelen = 0;
    unsigned finished = 0;
    FileData fileDataStart, fileDataEnd;
    FILE* fileFD;
    while(!finished){
        MessageInfo info = readMessage(fd, buffer);
        if(info.type != DATA){
            printf("Ocorreu um erro\n");
            break;
        }
        switch(info.data[0]){
            case 1:
                //Data packet
                writeToFile(info.data, info.dataSize);
                break;
            case 2:
                //Control packet begin
                fileDataStart = readControlPacket(info.data, info.dataSize);
                fileFD = fopen(fileData.filename, "w+");
                fclose(fileFD);
                break;
            case 3:
                //Control packet end
                fileDataEnd = readControlPacket(info.data, info.dataSize);
                finished = 1;
                break;
        }

    }

    printf("Closing port...\n");
    llclose(fd, 0);
    printf("Successfully closed port\n");
}

FileData readControlPacket(unsigned char *packet, unsigned int nBytes){
    
}

writeToFile(unsigned char* data, unsigned int dataSize, FileData fileData){
    FILE* fileFD = fopen(fileData.filename, "ab");
    fwrite(data, 1, dataSize, , fileFD);
    fclose(fileFD);
}