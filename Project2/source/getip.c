#include "getip.h"

int getIp(char* host) {
	struct hostent *h;

  if ((h=gethostbyname(host)) == NULL) {
    herror("gethostbyname");
    exit(1);
  }

  printf("Host name  : %s\n", h->h_name);
  printf("IP Address : %s\n",inet_ntoa(*((struct in_addr *)h->h_addr)));

  return 0;
}
