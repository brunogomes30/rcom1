#include "application.h"
#include "datalink.h"

int writeFile(char *file, char *port){
    printf("Openning port...\n");
    int fd = llopen(port, 1);
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
    FILE* fileFD = fopen(file, "rb");
    unsigned char buffer[MAX_PER_PACKET];
    unsigned char dataPacket[MAX_SIZE];
    unsigned long bytesWritten = 0;
    unsigned char nseq = 0;
    while(bytesWritten < filelen){
        nseq = (nseq + 1) % 255;
        unsigned char readInThisPacket = filelen - bytesWritten < MAX_PER_PACKET ? filelen - bytesWritten : MAX_PER_PACKET;
        //buffer[2] = 
        fread(buffer, 1, readInThisPacket, fileFD);
        bytesWritten += readInThisPacket;
        //Send packet
        int res, ntries = 0;
        unsigned int dataPacketSize = writeDataPacket(dataPacket, buffer, readInThisPacket, nseq);
        do{
            ntries ++;
            printf("Writting %d bytes... try nº%d", readInThisPacket, ntries);
            res = writeData(fd, dataPacket, dataPacketSize);
            if(res == -1){
                printf("#### trying again\n");
                continue;
            } else {
                break;
            }
        } while(ntries < 5);
        printf("proceeding to next packet\n");
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
        if(file[i] == '/') filename = file + 1 + i;
    }
    
    unsigned filenameSize = strlen(filename);
    fileData.filenameLen = filenameSize;
    //unsigned char buffer[MAX_SIZE];
    stat(file, &st);
    fileData.filenameLen = strlen(filename);
    fileData.filename = (char *) malloc(strlen(filename) * 1);
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
    for(int i=0; i < 4; i++){
        buffer[nBytes++] = (4 >> (8 * i)) & 0xff;
    }
    buffer[nBytes++] = 1;
    buffer[nBytes++] = fileData.filenameLen;
    for(int i=0; i < fileData.filenameLen; i++){
        buffer[nBytes++] = fileData.filename[i];
    }
    writeData(fd, buffer, nBytes);
    return 1;
}

unsigned int writeDataPacket(unsigned char *dataPacket , unsigned char* data, unsigned int size, unsigned char nseq){
    dataPacket[0] = 1;
    dataPacket[1] = nseq;
    dataPacket[2] = (size >> 8) & 0xff;
    dataPacket[3] = size & 0xff;
    memcpy(dataPacket + 4, data, size);
    return size + 4;
}

int readFile(char * port){
    unsigned S = 0;
    printf("openning port\n");
    int fd = llopen(port, 0);
    printf("opened successfully\n");

    //unsigned char buffer[MAX_SIZE];
    //unsigned long filelen = 0;
    unsigned finished = 0;
    FileData fileDataStart, fileDataEnd;
    FILE* fileFD;
    while(!finished){
        printf("######Ready to read next data...\n\n");
        MessageInfo info = readMessage(fd);
        S = info.s == 1 ? 0 : 1;
        if(info.type != DATA){
            printf("Ocorreu um erro\n");
            writeMessage(fd, A_EMISSOR, C_REJ(S));
            break;
        }
        printf("Data sent with S = %d\n", info.s);
        
        changeS(S);
        writeMessage(fd, A_EMISSOR, C_RR(S));
        printf("Switch case info.data[0] = %d\n", info.data[0]);
        switch(info.data[0]){
            case 1:
                printf("case 1\n\n");
                //Data packet
                writeToFile(info.data, info.dataSize, fileDataStart);
                break;
            case 2:
                printf("case 2\n");
                //Control packet begin
                fileDataStart = readControlPacket(info.data, info.dataSize);
                fileFD = fopen((char *) fileDataStart.filename, "w+");
                fclose(fileFD);
                break;
            case 3:
                printf("case 3\n");
                //Control packet end
                fileDataEnd = readControlPacket(info.data, info.dataSize);


                //aqui comparar com o fileDataStart
                finished = 1;
                break;
        }

    }

    if(fileDataEnd.filelen != fileDataStart.filelen){
        printf("error control packet end and begin\n");
        return -1;
    }
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
        printf("Reading start control packet");
    }
    else if (packet[currByte] == 3){
        printf("Reading end control packet");
    }
    else {
        perror("Wrong C in control packet");
    }
    currByte++;
    fileData.filelen = 0;
    while (currByte <= nBytes){
        type = packet[currByte++];
        typeSize = packet[currByte++];
        
        if (type == 0){
            for (unsigned i = 0; i <= typeSize; i++){
                fileData.filelen += packet[currByte++] << (typeSize - i)*8; 
            }
        }
        else if (type == 1){
            fileData.filenameLen = typeSize;
            fileData.filename = (char*) malloc(typeSize * 1);
            memcpy(fileData.filename, packet + currByte, typeSize);
            //strcpy( (char *) fileData.filename, packet + currByte); 
        }
        else {
            perror("Undefined type for control packet");
        }
        currByte += typeSize;
    } 
    printf("Filelen = %lu\n", fileData.filelen);
    printf("Filename len = %u\n", fileData.filenameLen);
    printf("Filename = %s\n", fileData.filename);
    return fileData;
}

int writeToFile(unsigned char* data, unsigned int dataSize, FileData fileData){
    FILE* fileFD = fopen((char *) fileData.filename, "ab");
    fwrite(data, dataSize, 1, fileFD);
    fclose(fileFD);
    return 0;
}



