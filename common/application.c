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
    unsigned char buffer[MAX_SIZE];
    unsigned char dataPacket[MAX_SIZE];
    unsigned long bytesWritten = 0;
    unsigned char nseq = 0;
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
        unsigned int dataPacketSize = writeDataPacket(dataPacket, buffer, readInThisPacket, nseq);
        do{
            ntries ++;
            printf("Writting %d bytes... try nº%d\n", readInThisPacket, ntries);
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
        if(file[i] == '/' ) filename = file + 1 + i;
    }
    

    /*
    char * getFilename(char * path) {
    char * filename = path, *p;
    for (p = path; *p; p++) {
        if (*p == '/' || *p == '\\' || *p == ':') {
            filename = p;
        }
    }
    return filename;
}
*/




    //unsigned filenameSize = strlen(filename);
    //fileData.filenameLen = filenameSize;
    fileData.filenameLen = strlen(filename);
    //unsigned char buffer[MAX_SIZE];
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
    //for(int i=0; i < 4; i++){
    //    buffer[nBytes++] = (4 >> (8 * i)) & 0xff;
    //}
    buffer[nBytes++] = 1;
    buffer[nBytes++] = fileData.filenameLen + 1;
    for(int i=0; i < fileData.filenameLen; i++){
        buffer[nBytes++] = fileData.filename[i];
    }
    buffer[nBytes++] = 0;
    writeData(fd, buffer, nBytes);
    return 1;
}

unsigned int writeDataPacket(unsigned char *dataPacket , unsigned char* data, unsigned int size, unsigned char nseq){
    dataPacket[0] = 1;
    dataPacket[1] = nseq;
    memcpy(dataPacket + 2, &size, 2);
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
    MessageInfo info;
    while(!finished){
        printf("######Ready to read next data...\n\n");
        unsigned tries = 0;
        do{
            info = readMessage(fd);
            if(info.type != DATA){
                printf("Ocorreu um erro\n");
                writeMessage(fd, A_EMISSOR, C_REJ(S));
                tries++;
            }else {
                S = info.s == 1 ? 0 : 1;
                break;
            }
        }while( tries < 3);
        //printf("Data sent with S = %d\n", info.s);
        
        changeS(S);
        writeMessage(fd, A_EMISSOR, C_RR(S));
        switch(info.data[0]){
            case 1:
                //printf("case 1\n\n");
                //Data packet
                //printBuffer(info.data + 4, info.dataSize - 4);
                writeToFile(info.data + 4, info.dataSize - 4, fileDataStart);
                break;
            case 2:
                //Control packet begin
                fileDataStart = readControlPacket(info.data, info.dataSize);
                fileFD = fopen((char *) fileDataStart.filename, "w+");
                fclose(fileFD);
                break;
            case 3:
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
    while (currByte < nBytes){
        printf("Start read control packet %u\n", currByte);
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
    printf("Filelen = %lu\n", fileData.filelen);
    printf("Filename len = %u\n", fileData.filenameLen);
    printf("Filename = %s\n", fileData.filename);
    return fileData;
}

int writeToFile(unsigned char* data, unsigned int dataSize, FileData fileData){
    
    FILE* fileFD = fopen(fileData.filename, "ab");
    fwrite(data, 1, dataSize, fileFD);
    fclose(fileFD);
    
    return 0;
}



