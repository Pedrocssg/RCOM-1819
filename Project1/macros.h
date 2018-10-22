#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4

#define FLAG 0x7E
#define A 0x03
#define A_ALT 0x01
#define UA_C 0x07
#define SET_C 0x03
#define RR_C_N0 0x05
#define RR_C_N1 0x85
#define REJ_C_N0 0x01
#define REJ_C_N1 0x81
#define DISC_C 0x0B
#define SET_BCC (A^SET_C)
#define UA_BCC (A^UA_C)
