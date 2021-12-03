#include "common.h"


/**
 * @brief Writes data, until a RR message is received
 * 
 * @param fd 
 * @param data 
 * @param nBytes 
 */
void writeData(int fd, unsigned char *data, unsigned nBytes);

/**
 * @brief Build data frame, with stuffed data, returns dataFrameSize
 * 
 * @param fd 
 * @param data 
 * @param nBytes 
 */
void buildDataFrame(unsigned char* data, unsigned nBytes); 

/**
 * @brief Builds data frame without the last BCC and flag
 * 
 * @param address 
 * @param S 
 * @param dataFrame 
 * @return unsigned 
 */
unsigned buildDataFrame(unsigned char address, unsigned char S, unsigned char *dataFrame);

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
void writeMessage(unsigned char address, unsigned char C); 

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
unstuffData(unsigned char * stuffedData,unsigned char *data, unsigned stuffedDataSize);

