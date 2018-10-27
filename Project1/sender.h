#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "macros.h"

typedef struct {
    int fd;
    struct termios oldtio, newtio;
} ApplicationLayer;

int llopen(ApplicationLayer *appLayer);
int createBoundFrame(unsigned char *bound, long fileSize, const char *fileName, unsigned char frame);
int createInfoFrame(unsigned char *message, int messageSize, unsigned char *infoFrame);
int byteStuffing(unsigned char* frame, int frameSize);
int llwrite(int fd, unsigned char *buf, int length);
int llclose(ApplicationLayer *appLayer);
int stateMachineSupervision(int port, int *state, unsigned char *frame);
int stateMachineInfoAnswer(int port, int *state);
