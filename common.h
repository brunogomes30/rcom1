#define FLAG  0b01111110
#define A_EMISSOR  0b00000011
#define A_RECETOR  0b00000001
#define C_SET  0b00000011
#define C_DISC  0b00001011
#define C_UA  0b00000111
#define C_RR  0b00000101
#define C_REJ  0b00000001

typedef enum  {START, FLAG_RECV, A_RECV, C_RECV, BCC_OK, STOP_STATE} State;


int isC(char byte);
int checkBCC(char byte, char *msg);
State changeState(char byte, State currentState, char *msg);
