#include "macros.h"
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

typedef struct{
    int fd;
    struct termios oldtio, newtio;
} ApplicationLayer;

volatile int STOP=FALSE;
int llopen(ApplicationLayer *appLayer);
int llread(int port, unsigned char *data);
int llclose(ApplicationLayer *appLayer);
int processInfoFrame(int port, unsigned char * data, unsigned char c);
int processBoundFrame(int port, unsigned char * data, unsigned char c);
int getFileName(unsigned char * data, char * fileName);
int getFileSize(unsigned char * data, int * fileSize);
int writeFileData(unsigned char * data, int fd);
int stateMachineSupervision(int port, int *state, unsigned char *frame);
