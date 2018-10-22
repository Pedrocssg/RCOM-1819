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
int llwrite(int fd, char *buf, int length);
int llclose(ApplicationLayer *appLayer);
int stateMachineSupervision(int port, int *state, unsigned char *frame);
