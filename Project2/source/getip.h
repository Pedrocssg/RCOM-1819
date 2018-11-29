#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>


typedef struct {
  char *h_name;	//Official name of the host.
  char **h_aliases;	//A NULL-terminated array of alternate names for the host.
  int h_addrtype;	//The type of address being returned; usually AF_INET.
  int h_length;	//The length of the address in bytes.
  char **h_addr_list;	//A zero-terminated array of network addresses for the host.
  //Host addresses are in Network Byte Order.
} hostent;

#define h_addr h_addr_list[0]	//The first address in h_addr_list.

int getIp(char* host);
