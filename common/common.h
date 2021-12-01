#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define FLAG  0b01111110
#define A_EMISSOR  0b00000011
#define A_RECETOR  0b00000001
#define C_SET  0b00000011
#define C_DISC  0b00001011
#define C_UA  0b00000111
#define C_RR  0b00000101
#define C_REJ  0b00000001
#define ESCAPE 0b01111110
#define MAX_SIZE 100
#define BIT(n) 1 << n

#define BAUDRATE B38400



typedef enum  {START, FLAG_RECV, A_RECV, C_RECV, BCC_OK, STOP_STATE} State;
typedef enum  {DATA, CONTROL, ERROR} MessageType;
typedef struct {
    MessageType type;
    char *data;
    int nBytes;
    int s;
} MessageInfo;

typedef struct {
    unsigned char s;
    
} ApplicationData;

void printBuffer(unsigned char * buffer, unsigned size);
int isC(char byte, char S);
int checkBCC(char byte, char *msg);
State changeState(char byte, State currentState, char *msg, char S);
MessageInfo readMessage(int fd,  ApplicationData *appdata);
int llopen(char *port, int isTransmitter, ApplicationData *appdata);
int llclose(int fd, int isTransmitter, ApplicationData *appdata);
int llread(int fd, char *buffer,  ApplicationData *applicationData);
int llwrite(int fd, char *buffer, int length);
int writeData(int fd, char *data, int nBytes,  ApplicationData *applicationData);
int readData(int fd, char *data, State *state);
void writeMessage(int fd, unsigned char address, unsigned char C);
int writeFile(int fd, char* path,  ApplicationData *applicationData);




