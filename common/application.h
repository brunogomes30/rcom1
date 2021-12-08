#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include "datalink.h"

typedef struct{
    unsigned long filelen;
    char *filename;
    unsigned int filenameLen; 
}FileData;

int writeFile(char *path, char* port);


FileData readFileData(char *file);
int writeControlPacket(FileData fileData, int fd, int isStart);
unsigned int writeDataPacket(unsigned char *dataPacket , unsigned char* data, unsigned int size, unsigned char nseq);
//FileData readControlPacket(unsigned char *packet);
int readDataPacket(unsigned char *packet, unsigned int nBytes);

FileData readControlPacket(unsigned char *packet, unsigned int nBytes);

int writeToFile(unsigned char* data, unsigned int dataSize, FileData fileData);

int readFile(char * port);


