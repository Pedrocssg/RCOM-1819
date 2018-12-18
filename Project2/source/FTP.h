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


#define FTP_PORT             21
#define READY                "220"
#define SPECIFY_PASSWORD     "331"
#define LOGIN_SUCCESSFUL     "230"
#define PASSIVE_MODE         "227"
#define BINARY_MODE          "150"
#define TRANSFER_SUCCESSFUL  "226"
#define GOODBYE              "221"

int connectSocket();
int readFTP(int fd, char * code);
int sendFTP(int fd);
int loginFTP();
int connectFTP();
int enterPassiveMode();
int retrieveFTP();
int download();
int quit();


#endif
