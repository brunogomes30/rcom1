#ifndef DATALINK_H
#define DATALINK_H
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

#define FLAG  0b01111110
#define A_EMISSOR  0b00000011
#define A_RECETOR  0b00000001
#define C_SET  0b00000011
#define C_DISC  0b00001011
#define C_UA  0b00000111
#define C_RR(S)  (0x5 |  (S << 7))
#define C_REJ(S)  (0x1 | (S << 7))
#define ESCAPE 0b01111110
#define MAX_PER_PACKET 100
#define MAX_SIZE 300
#define BIT(n) 1 << n

#define BAUDRATE B38400



typedef enum  {START, FLAG_RECV, A_RECV, C_RECV, BCC_OK, STOP_STATE} State;
typedef enum  {DATA, CONTROL, ERROR} MessageType;
typedef struct {
    MessageType type;
    unsigned char *data;
    int nBytes;
    unsigned dataSize;
    int s;
} MessageInfo;

/**
 * @brief Returns fd
 * 
 * @return int 
 */
int llopen(char *port, int isTransmitter);

/**
 * @brief closes port
 * 
 * @return int 
 */
int llclose(int fd, int isTransmitter);


/**
 * @brief Writes data, until a RR message is received
 * 
 * @param fd 
 * @param data 
 * @param nBytes 
 */
int writeData(int fd, unsigned char *data, unsigned nBytes);

/**
 * @brief Build data frame, with stuffed data, returns dataFrameSize
 * 
 * @param fd 
 * @param data 
 * @param nBytes 
 */
unsigned buildDataFrame(unsigned char* data, unsigned nBytes, unsigned char *dataFrame); 

/**
 * @brief Builds data frame without the last BCC and flag
 * 
 * @param address 
 * @param S 
 * @param dataFrame 
 * @return unsigned 
 */
//unsigned buildDataFrame(unsigned char address, unsigned char S, unsigned char *dataFrame);

/**
 * @brief Stuff data, returns size of stuffedData
 * 
 * @param data 
 * @param nBytes 
 * @param stuffedData 
 * @return unsigned 
 */
unsigned stuffData(unsigned char *data, unsigned nBytes, unsigned char *stuffedData);




/**
 * @brief Write control frame, C already contains the R 
 * 
 * @param address 
 * @param C 
 */
void writeMessage(int fd, unsigned char address, unsigned char C); 

/**
 * @brief Read message.
 *             returns messageInfo.type = ERROR on error
 * 
 * @param fd 
 * @return MessageInfo 
 */
MessageInfo readMessage(int fd);

/**
 * @brief Unstuff data, returns unstuffed data size
 * 
 * @param stuffedData 
 * @param data 
 * @param stuffedDataSize 
 */
unsigned unstuffData(unsigned char * stuffedData,unsigned char *data, unsigned stuffedDataSize, unsigned char givenBCC);


State changeState(unsigned char byte, State currentState, unsigned char *msg, char S);


int checkBCC(unsigned char byte, unsigned char *msg);

int isC(unsigned char byte, char S);

void changeS(unsigned char v);

#endif
