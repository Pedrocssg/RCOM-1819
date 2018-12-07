#include "getip.h"

void getIp(char * ip, char * host) {
	struct hostent *h;

  if ((h=gethostbyname(host)) == NULL) {
    herror("gethostbyname");
    exit(1);
  }

  strcpy(ip, inet_ntoa(*((struct in_addr *)h->h_addr)));
}
