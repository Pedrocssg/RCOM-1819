#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include "macros.h"


typedef struct {
    char port[20];
    int baudRate;
    unsigned int sequenceNumber;
    unsigned int timeout;
    unsigned int numTansmissions;
    char frame[MAX_FRAME_SIZE];
} LinkLayer;


int llopen(int port, int status);
int llopenReceiverHandler(int port);
int llopenTransmitterHandler(int port);

int createBoundFrame(unsigned char *bound, long fileSize, const char *fileName, unsigned char frame);
int createInfoFrame(unsigned char *message, int messageSize, unsigned char *infoFrame);
int byteStuffing(unsigned char* frame, int frameSize);

int llwrite(int port, unsigned char *buf, int length);
int stateMachineInfoAnswer(int port, int *state);

int llread(int port, unsigned char *data);
int processInfoFrame(int port, unsigned char * data, unsigned char c);
int processBoundFrame(int port, unsigned char * data, unsigned char c);

int llclose(int port);

int stateMachineSupervision(int port, int *state, unsigned char *frame);
