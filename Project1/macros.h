#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define TRANSMITTER 0
#define RECEIVER 1

#define FALSE 0
#define TRUE 1
#define LONG_SIZE 4
#define BYTE_SIZE 255
#define PACKET_HEADER 5
#define FRAME_HEADER 5
#define HEADER PACKET_HEADER+FRAME_HEADER
#define ESC 0x7d

#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define DATA_OK 5

#define FLAG 0x7e

#define A 0x03
#define A_ALT 0x01

#define UA_C 0x07
#define SET_C 0x03
#define RR_C_N0 0x05
#define RR_C_N1 0x85
#define REJ_C_N0 0x01
#define REJ_C_N1 0x81
#define I0_C 0x00
#define I1_C 0x40
#define DISC_C 0x0b

#define SET_BCC (A^SET_C)
#define UA_BCC_REC (A^UA_C)
#define UA_BCC_EM (A_ALT^UA_C)
#define DISC_BCC_REC (A_ALT^DISC_C)
#define DISC_BCC_EM (A^DISC_C)
#define RR0_BCC (A^RR_C_N0)
#define RR1_BCC (A^RR_C_N1)
#define REJ0_BCC (A^REJ_C_N0)
#define REJ1_BCC (A^REJ_C_N1)

#define START_FRAME 0x02
#define INFO_FRAME 0x01
#define END_FRAME 0x03
#define T1 0x00
#define T2 0x01
