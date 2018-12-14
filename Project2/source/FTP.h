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


#define FTP_PORT              21
#define SERVER_READY          "220"
#define DOWNLOAD_SUCCESSFULL  "226"
#define PASSIVE               "227"
#define LOGIN_SUCCESSFULL     "230"
#define PASSSWORD_REQUIRED    "331"

int connectSocket();
int readFTP(char * code);
int sendFTP();
int loginFTP();
int connectFTP();
int enterPassiveMode();


#endif
