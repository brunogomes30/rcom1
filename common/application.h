
typedef struct{
    unsigned long filelen;
    unsigned char *filename;
    int filenameLen; 
}FileData;

int writeFile(char *path);


FileData readFileData(char *file);
int writeControlPacket(FileData fileData, int fd);

int readControlPacket(unsigned char *packet);
int readDataPacket(unsigned char *packet);

FileData readControlPacket(unsigned char *packet, unsigned int nBytes);

