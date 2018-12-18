#include "FTP.h"

URL * url;
int controlSocket;
int dataSocket;
char string[COMMAND_SIZE];

int main(int argc, char** argv){
	if (argc != 2) {
			printf("Please insert a valid url: './download ftp://[<user>:<password>@]<host>/<url-path>'\n");
			exit(1);
	}

	url = malloc(sizeof(URL));

	if(parseURL(argv[1], url) == -1)
			exit(1);

	if(getIP(url) == -1)
			exit(1);

	if(connectToFTP() == -1)
			exit(1);

	if(login() == -1)
			exit(1);

	if(passive() == -1)
			exit(1);

	if(filesize() == -1)
			exit(1);

	if(retrieve() == -1)
			exit(1);

	if(download() == -1)
			exit(1);

	if(quit() == -1)
			exit(1);

	exit(0);
}

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

int getIP(URL * url) {
	struct hostent *h;

  if ((h=gethostbyname(url->host)) == NULL) {
    printf("Error connecting to host\n");
    return -1;
  }

  strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));

	return 0;
}

int connectToFTP(){
	if((controlSocket = connectSocket(url->ip, url->port)) == 0)
			return -1;

	if(ftpRead(controlSocket, READY, TRUE) == -1)
			return -1;

	return 0;
}

int login(){
	sprintf(string, "USER %s\n", url->user);

	if(ftpSend(controlSocket) == -1)
			return -1;

	if(ftpRead(controlSocket, SPECIFY_PASSWORD, TRUE) == -1)
			return -1;

	sprintf(string, "PASS %s\n", url->password);

	if(ftpSend(controlSocket) == -1)
			return -1;

	if(ftpRead(controlSocket, LOGIN_SUCCESSFUL, TRUE) == -1)
			return -1;

	return 0;

}

int passive(){
	sprintf(string, "PASV\n");

	if(ftpSend(controlSocket) == -1)
			return -1;

	if(ftpRead(controlSocket, PASSIVE_MODE, TRUE) == -1)
			return -1;

	int ip[6];
	sscanf(string, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip[0], &ip[1], &ip[2], &ip[3], &ip[4], &ip[5]);
	sprintf(url->ip, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	url->port = ip[4]*256 + ip[5];

	if((dataSocket = connectSocket(url->ip, url->port)) == 0)
			return -1;


	return 0;
}

int filesize(){
	sprintf(string, "SIZE %s\n", url->path);

	if(ftpSend(controlSocket) == -1)
			return -1;

	if(ftpRead(controlSocket, FILESIZE, FALSE) == -1){
			printf("550 File not found\n");
			return -1;
	}

	sscanf(string, "213 %d", &url->filesize);

	return 0;
}

int retrieve(){
	sprintf(string, "RETR %s\n", url->path);

	if(ftpSend(controlSocket) == -1)
			return -1;

	if(ftpRead(controlSocket, BINARY_MODE, TRUE) == -1)
			return -1;

	return 0;
}

int download(){
	FILE * output;
	output = fopen(url->filename, "w");

	char buf[32768];
	int data, size = 0;

	printf("000 Transfering %s\n", url->filename);

	while((data = read(dataSocket, buf, sizeof(buf))) != 0){
		fwrite(buf, data, 1, output);
		size += data;
		progress(size);
	}

	printf("\n");

	if(ftpRead(controlSocket, TRANSFER_SUCCESSFUL, TRUE) == -1)
			return -1;

	fclose(output);

	return 0;
}

int quit(){
	sprintf(string, "QUIT\n");

	if(ftpSend(controlSocket) == -1)
			return -1;

	if(ftpRead(controlSocket, GOODBYE, TRUE) == -1)
			return -1;

	close(controlSocket);
	close(dataSocket);

	free(url);

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

int ftpSend(int fd){
	int info;

	if ((info = write(fd, string, strlen(string))) <= 0)
			return -1;

	return 0;
}

int ftpRead(int fd, char * code, int print) {
	FILE* file = fdopen(fd, "r");

	do {
			fgets(string, sizeof(string), file);
			if(print)
					printf("%s", string);
	} while (!('1' <= string[0] && string[0] <= '5') || string[3] != ' ');

	if(strncmp(code, string, strlen(code)) != 0)
			return -1;

	return 0;
}

void progress(int size){
	int progress = (int)(((double)size/url->filesize)*100);
	int i;

	printf("\r");
	printf("000 [");

	for(i = 0; i < progress/2; i++)
			printf("#");

	for(i = 0; i < (50-progress/2); i++)
			printf(".");

	printf("]");
	printf(" %d%%",progress);

	fflush(stdout);
}
