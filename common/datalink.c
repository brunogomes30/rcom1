#include "datalink.h"

static unsigned char S = 0;

void changeS(unsigned char v){
    S = v;
}

void printBuffer(unsigned char *buffer, unsigned size)
{
    printf("Buffer = \n");
    for (int i = 0; i < size; i++)
    {
        printf("%x ", buffer[i]);
    }
    printf("\n");
}

int llopen(char *port, int isTransmitter)
{
    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(port);
        return -1;
    }
    struct termios oldtio, newtio;
    if (tcgetattr(fd, &oldtio) == -1)
    { /* save current port settings */
        perror("tcgetattr");
        // exit(-1);
        return -1;
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 5; /* inter-character timer unused */
    newtio.c_cc[VMIN] = 0;  /* blocking read until 5 chars received */

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }


    MessageInfo info;
    info.type = DATA;
    unsigned tries = 0, maxtries = 3;
    do{
        if (isTransmitter)
        {   
            writeMessage(fd, A_EMISSOR, C_SET);
            info = readMessage(fd);
            if(info.type == ERROR){
                tries++;
            } else {
                break;
            }
        }
        else {
            info = readMessage(fd);
            if (info.type == CONTROL && info.data[0] == C_SET) {
                writeMessage(fd, A_EMISSOR, C_UA);
                break;
            } else {
                tries++;
            }
        }
    }while(tries < maxtries);
    if (info.type == ERROR || tries == 3)
    {
        return -1;
    }
    return fd;
}



int writeData(int fd, unsigned char *data, unsigned nBytes)
{
    unsigned char dataFrame[MAX_SIZE];
    unsigned dataFrameSize = buildDataFrame(data, nBytes, dataFrame);
    unsigned tries = 0;
    unsigned char exitWhile = 0;
    while (tries < 3)
    {
        write(fd, dataFrame, dataFrameSize);
        S = S == 1 ? 0 : 1;
        MessageInfo info = readMessage(fd);
        S = S == 1 ? 0 : 1;
        switch (info.type)
        {
        case ERROR:
            printf("message is error\n");
            tries++;
            break;
        case CONTROL:
            if (info.data[0] == C_RR(info.s))
            {
                S = info.s;
                exitWhile = 1;
            }else {
                //info.s = S;
                S = info.s;
                return -1;
            }
            break;
        case DATA:
            tries++;
            //Shouldn't happen
            break;
        }
        if (exitWhile)
            break;
    }

    if (tries == 3)
    {
        printf("Failed to write 3 times in a row\n");
        return -1;
    }
    return 0;
}

int llwrite(int fd, unsigned char *data, unsigned nBytes){
    return writeData(fd, data, nBytes);
}

int llread(int fd, unsigned char *data){
    MessageInfo info;
    unsigned tries = 0;
    do{
        info = readMessage(fd);
        if(info.type != DATA){
            writeMessage(fd, A_EMISSOR, C_REJ(S));
            tries++;
        }else {
            S = info.s == 1 ? 0 : 1;
            break;
        }
    }while( tries < 3);
    
    if(tries < 3){
        changeS(S);
        writeMessage(fd, A_EMISSOR, C_RR(S));
        memcpy(data, info.data, info.dataSize);
        free(info.data);
        return info.dataSize;
    } else {
        writeMessage(fd, A_EMISSOR, C_REJ(S));
        return -1;
    }
}

unsigned buildDataFrame(unsigned char *data, unsigned nBytes, unsigned char *dataFrame) {
    unsigned char buffer[MAX_SIZE];
    unsigned bufferSize = 0;

    buffer[0] = FLAG;
    buffer[1] = A_EMISSOR;
    buffer[2] = S == 1 ? BIT(6) : 0;
    buffer[3] = buffer[1] ^ buffer[2];

    unsigned int dataFrameSize = 4;
    unsigned int stuffedDataSize = stuffData(data, nBytes, buffer + dataFrameSize);
    bufferSize = dataFrameSize + stuffedDataSize;

    // Build last BCC
    unsigned char bcc = 0;
    for (unsigned i = 0; i < nBytes; i++)
    {
        bcc = bcc ^ data[i];
    }
    if(bcc != FLAG){
        buffer[bufferSize++] = bcc;
    } else {
        buffer[bufferSize++] = ESCAPE;
        buffer[bufferSize++] = 0x5e;
    }
    buffer[bufferSize++] = FLAG;
    memcpy(dataFrame, buffer, bufferSize);
    return bufferSize;
}

unsigned stuffData(unsigned char *data, unsigned nBytes, unsigned char *stuffedData)
{
    unsigned stuffedSize = 0;
    for (int i = 0; i < nBytes; i++)
    {
        if (data[i] == FLAG) {
            stuffedData[stuffedSize++] = ESCAPE;
            stuffedData[stuffedSize++] = 0x5e;
        }
        else if (data[i] == ESCAPE) {
            stuffedData[stuffedSize++] = ESCAPE;
            stuffedData[stuffedSize++] = 0x5d;
        }
        else {
            stuffedData[stuffedSize++] = data[i];
        }
    }

    return stuffedSize;
}

void writeMessage(int fd, unsigned char address, unsigned char C)
{
    unsigned char buffer[5];
    buffer[0] = FLAG;
    buffer[1] = address;
    buffer[2] = C;
    buffer[3] = address ^ C;
    buffer[4] = FLAG;
    write(fd, buffer, 5);
}

MessageInfo readMessage(int fd)
{
    unsigned char buffer[MAX_SIZE];
    unsigned bufferSize = 0;
    State state = START;
    unsigned stuffedDataSize = 0;
    MessageInfo info;
    unsigned tries = 0;
    while (state != STOP_STATE && tries < 5) {
        int res = read(fd, buffer + (bufferSize), 1);
        if (res == 0) {
            tries++;
        }

        if (!(state == BCC_OK && info.type == DATA))
            state = changeState(buffer[bufferSize], state, buffer, S);
        else if (buffer[bufferSize] == FLAG) {
            state = STOP_STATE;
            break;
        }
        if (state == FLAG_RECV) {
            buffer[0] = FLAG;
            bufferSize = 1;
            stuffedDataSize = 0;
        }
        else if (state == START)
        {
            bufferSize = 0;
        }

        if (state == C_RECV)
        {
            unsigned isData = buffer[bufferSize] == (S == 1 ? BIT(6) : 0);
            info.type = isData ? DATA : CONTROL;
            if (info.type == CONTROL) {
                info.s = (buffer[2] & BIT(7)) == BIT(7) ? 1 : 0;
            }
            else {
                info.s = buffer[bufferSize] == BIT(6) ? 1 : 0;
            }
        }
        if (state == BCC_OK)
        {
            if (info.type == DATA)
            {
                stuffedDataSize++;
            }
            else if (info.type == CONTROL)
            {
                info.data = (unsigned char *)malloc(1 * sizeof(unsigned char));
                info.dataSize = 0;
                info.data[0] = buffer[2];
            }
        }
        bufferSize++;
    }

    if (state != STOP_STATE) {
        info.type = ERROR;
        info.dataSize = 0;
        info.s = S;
        return info;
    }
    if (info.type == CONTROL)
    {
        return info;
    }

    unsigned char stuffedData[stuffedDataSize];
    stuffedDataSize -= 2;
    memcpy(stuffedData, buffer + 5, stuffedDataSize);
    unsigned char data[stuffedDataSize];
    unsigned char givenBCC;
    if(buffer[stuffedDataSize + 5] == 0x5e){
        givenBCC = buffer[stuffedDataSize + 4] == 0x7d ? FLAG : 0x5e;
    } else {
        givenBCC = buffer[stuffedDataSize + 5];
    }
    
    //Used to create random errors
    if(rand() % 30 == 0){
        givenBCC -= 1;
    }
    
    unsigned dataSize = unstuffData(stuffedData, data, stuffedDataSize, givenBCC);
    if (dataSize == 0)
    {
        // Try again
        printf("######\n###BCC error\n######\n");
        info.type = ERROR;
        info.s = S;
        info.dataSize = 0;
        return info;
    }
    info.data = (unsigned char *) malloc(sizeof(unsigned char) * dataSize);
    info.dataSize = dataSize;
    memcpy(info.data, data, dataSize);

    return info;
}

unsigned unstuffData(unsigned char *stuffedData, unsigned char *data, unsigned stuffedDataSize, unsigned char givenBCC)
{
    unsigned bcc = 0;
    unsigned dataSize = 0;
    unsigned previousIsEscape = 0;
    // Verificar se stuffedData n??o ?? flag
    for (unsigned i = 0; i < stuffedDataSize; i++)
    {
        unsigned char c = stuffedData[i];
        if (previousIsEscape)
        {
            previousIsEscape = 0;
            if (c == 0x5e)
            {
                c = FLAG;
            }
            else if (c == 0x5d)
            {
                c = ESCAPE;
            }
            else
            {
                return 0;
            }
        }
        else if (c == ESCAPE)
        {
            previousIsEscape = 1;
        }
        else if (c == FLAG)
        {
            return 0;
        }
        if (!previousIsEscape) {
            data[dataSize++] = c;
            bcc = bcc ^ c;
        }
    }
    if (bcc != givenBCC)
    {
        //printf("\nCalculated bcc = %x, given bcc = %x \n", bcc, givenBCC);
        return 0;
    }
    return dataSize;
}

int llclose(int fd, int isTransmitter)
{
    if (isTransmitter)
    {
        writeMessage(fd, A_EMISSOR, C_DISC);
        MessageInfo info = readMessage(fd);
        if (info.type == ERROR)
        {
            return -1;
        }
        if (info.type == CONTROL && info.data[0] == C_DISC)
        {
            writeMessage(fd, A_RECETOR, C_UA);
        }
        close(fd);
    }
    else
    {
        MessageInfo info = readMessage(fd);
        if (info.type == ERROR)
        {
            return -1;
        }
        if (info.type == CONTROL && info.data[0] == C_DISC)
        {
            writeMessage(fd, A_EMISSOR, C_DISC);
        }
    }
    return 0;
}

State changeState(unsigned char byte, State currentState, unsigned char *msg, char S)
{
    switch (currentState)
    {
    case START:
        if (byte == FLAG)
        {
            currentState = FLAG_RECV;
            msg[0] = byte;
        }
        break;
    case FLAG_RECV:
        if (byte == FLAG)
        {
            currentState = FLAG_RECV;
            msg[0] = byte;
        }
        else if (byte == A_EMISSOR || byte == A_RECETOR)
        {
            currentState = A_RECV;
            msg[1] = byte;
        }
        else
        {
            currentState = START;
        }
        break;
    case A_RECV:
        if (isC(byte, S))
        {
            currentState = C_RECV;
            msg[2] = byte;
        }
        else if (byte == FLAG)
        {
            msg[0] = byte;
            currentState = FLAG_RECV;
        }
        else
        {
            currentState = START;
        }
        break;
    case C_RECV:
        if (checkBCC(byte, msg))
        {
            msg[3] = byte;
            currentState = BCC_OK;
        }
        else if (byte == FLAG)
        {
            msg[0] = byte;
            currentState = FLAG_RECV;
        }
        else
        {
            currentState = START;
        }
        break;
    case BCC_OK:
        if (byte == FLAG)
        {
            msg[4] = byte;
            currentState = STOP_STATE;
        }
        else
        {
            currentState = START;
        }
        break;
    case STOP_STATE:
        break;
    }
    return currentState;
}

int checkBCC(unsigned char byte, unsigned char *msg)
{
    return byte == (msg[2] ^ msg[1]);
}

int isC(unsigned char byte, char S) {
    return byte == C_SET || byte == C_DISC || byte == C_UA || byte == C_RR(S) || byte == C_REJ(S) || byte == C_REJ(S == 1 ? 0 : 1) || byte == (S == 1 ? BIT(6) : 0);
}

