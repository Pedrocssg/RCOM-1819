#ifndef __FTP_H
#define __FTP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>

#define h_addr h_addr_list[0]	//The first address in h_addr_list.

#define TRUE                  1
#define FALSE                 0

#define URL_SIZE              255
#define COMMAND_SIZE          1024

#define FTP_PORT              21
#define READY                 "220"
#define SPECIFY_PASSWORD      "331"
#define LOGIN_SUCCESSFUL      "230"
#define PASSIVE_MODE          "227"
#define BINARY_MODE           "150"
#define FILESIZE              "213"
#define TRANSFER_SUCCESSFUL   "226"
#define GOODBYE               "221"

typedef struct {
    char user[URL_SIZE];
    char password[URL_SIZE];
    char host[URL_SIZE];
    char path[URL_SIZE];
    char ip[URL_SIZE];
    char filename[URL_SIZE];
    int filesize;
    int port;
} URL;

typedef struct {
  char *h_name;	//Official name of the host.
  char **h_aliases;	//A NULL-terminated array of alternate names for the host.
  int h_addrtype;	//The type of address being returned; usually AF_INET.
  int h_length;	//The length of the address in bytes.
  char **h_addr_list;	//A zero-terminated array of network addresses for the host.
  //Host addresses are in Network Byte Order.
} hostent;

int parseURL(char * urlString, URL * url);
int getIP(URL * url);
int connectToFTP();
int login();
int passive();
int filesize();
int retrieve();
int download();
int quit();

int parseFilename(URL * url);
int connectSocket();
int ftpSend(int fd);
int ftpRead(int fd, char * code, int print);
void progress(int size);

#endif
