#include "utils.h"

int parseURL(char * urlString, URL * url){
	url->port = FTP_PORT;

	if(sscanf(urlString, "ftp://%99[^:]:%99[^@]@%99[^/]/%99[^\n]", url->user, url->password, url->host, url->path) != 4){
			if(sscanf(urlString, "ftp://%99[^/]/%99[^\n]",url->host, url->path) != 2){
					printf("%s000%s Error while parsing URL.\n", RED, RESET);
					printf("%s000%s Please enter in this format 'ftp://user:password@host/filepath'.\n", RED, RESET);
					return -1;
			} else{
				strcpy(url->user, "ftp");
				strcpy(url->password, "ftp");
			}
	}

	printf("%s000%s Username: %s\n", BLUE, RESET, url->user);
	printf("%s000%s Password: %s\n", BLUE, RESET, url->password);
	printf("%s000%s Host: %s\n", BLUE, RESET, url->host);
	printf("%s000%s Path: %s\n", BLUE, RESET, url->path);

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
    printf("%s000%s Error connecting to host.\n", RED, RESET);
    return -1;
  }

  strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));

	printf("%s000%s IP: %s\n", BLUE, RESET, url->ip);
	printf("%s000%s Port: %d\n", BLUE, RESET, url->port);

	return 0;
}

int connectSocket(char * ip, int port) {
	int	socketfd;
	struct sockaddr_in server_addr;

	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);		/*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	if ((socketfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
			perror("socket()");
	    exit(0);
	}

	/*connect to the server*/
	if(connect(socketfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
	    perror("connect()");
			exit(0);
	}

	return socketfd;
}

void progress(URL * url, int size){
	int progress = (int)(((double)size/url->filesize)*100);
	int i;

	printf("\r");
	printf("%s000%s [", BLUE, RESET);

	for(i = 0; i < progress/2; i++)
			printf("%s#%s",PINK, RESET);

	for(i = 0; i < (50-progress/2); i++)
			printf(".");

	printf("]");
	printf(" %d%%",progress);

	fflush(stdout);
}
