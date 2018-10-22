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
int llread(int port, char *buf);
int llclose(ApplicationLayer *appLayer);
int stateMachineSupervision(int port, int *state, unsigned char *frame);
