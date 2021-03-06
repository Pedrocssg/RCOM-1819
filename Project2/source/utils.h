#ifndef __UTILS_H
#define __UTILS_H

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
#include "FTP.h"

#define h_addr h_addr_list[0]	//The first address in h_addr_list.

#define URL_SIZE              255

#define RED                   "\033[0;31m"
#define GREEN                 "\033[0;32m"
#define PINK                  "\033[1;35m"
#define BLUE                  "\033[0;36m"
#define RESET                 "\033[0m"

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
int parseFilename(URL * url);
int getIP(URL * url);
int connectSocket();
void progress(URL * url, int size);

#endif
