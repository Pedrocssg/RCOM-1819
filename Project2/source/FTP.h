#ifndef __FTP_H
#define __FTP_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include "URL.h"

#define TRUE  1
#define FALSE 0

#define FTP_PORT             21
#define READY                "220"
#define SPECIFY_PASSWORD     "331"
#define LOGIN_SUCCESSFUL     "230"
#define PASSIVE_MODE         "227"
#define BINARY_MODE          "150"
#define FILESIZE             "213"
#define TRANSFER_SUCCESSFUL  "226"
#define GOODBYE              "221"

int connectSocket();
int ftpRead(int fd, char * code, int print);
int ftpSend(int fd);
int login();
int connectFTP();
int passive();
int filesize();
int retrieve();
int download();
int quit();
void progressBar(int size);


#endif
