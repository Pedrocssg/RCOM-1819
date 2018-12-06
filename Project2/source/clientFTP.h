#ifndef __CLIENTFTP_H
#define __CLIENTFTP_H

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
#include "getip.h"

#define FTP_PORT 21

typedef struct {
    char user[255];
    char password[255];
    char host[255];
    char path[255];
    char ip[255];
    int port;
} URL;

int connectSocket();

#endif
