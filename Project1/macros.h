#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define A 0x03
#define UA_C 0x07
#define SET_C 0x03
#define SET_BCC (A^SET_C)
#define UA_BCC (A^UA_C)
