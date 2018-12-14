#include "URL.h"

int parseURL(char * urlString, URL * url){
	url->port = FTP_PORT;

	if(sscanf(urlString, "ftp://%99[^:]:%99[^@]@%99[^/]/%99[^\n]", url->user, url->password, url->host, url->path) != 4){
			if(sscanf(urlString, "ftp://%99[^/]/%99[^\n]",url->host, url->path) != 2){
					printf("Error while parsing URL. Please enter in this format 'ftp://user:password@host/filepath'\n");
					return -1;
			} else{
				strcpy(url->user, "ftp");
				strcpy(url->password, "ftp");
			}
	}

	parseFilename(url);

	return 0;
}

int parseFilename(URL * url){
	char * ret;

	if((ret = strrchr(url->path, '/')) == NULL){
		strcpy(url->filename, url->path);
	}
	else{
		ret++;
		memcpy(url->filename, ret, URL_SIZE);
	}

	return 0;
}

int getIP(URL * url) {
	struct hostent *h;

  if ((h=gethostbyname(url->host)) == NULL) {
    printf("Error connecting to host\n");
    return -1;
  }

  strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));

	return 0;
}
