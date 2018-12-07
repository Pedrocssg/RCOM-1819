#ifndef __FTP_H
#define __FTP_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include "URL.h"

#define FTP_PORT 21

int connectSocket();
int readFTP(char * string, size_t size);
int sendFTP(char * string, size_t size);
int loginFTP();
int connectFTP();


#endif
