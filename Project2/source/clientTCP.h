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

#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"

int parseFTPrequest(char *request, char *user, char *password, char *host, char *url);
int connectSocket();
