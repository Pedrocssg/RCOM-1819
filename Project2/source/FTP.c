#include "FTP.h"

URL * url;
int controlSocket;
int dataSocket;
char string[1024];

int main(int argc, char** argv){
	if (argc != 2) {
			printf("Please insert a valid url: './download ftp://(user:password@)host/filepath'\n");
			exit(1);
	}

	url = malloc(sizeof(URL));

	if(parseURL(argv[1], url) == -1)
			exit(1);

	if(getIP(url) == -1)
			exit(1);

	if(connectFTP() == -1)
			exit(1);

	if(loginFTP() == -1)
			exit(1);

	if(enterPassiveMode() == -1)
			exit(1);

	if(retrieveFTP() == -1)
			exit(1);

	if(download() == -1)
			exit(1);

	if(quit() == -1)
			exit(1);

	exit(0);
}

int quit(){
	sprintf(string, "QUIT\n");

	if(sendFTP(controlSocket))
			return -1;

	if(readFTP(controlSocket, GOODBYE) == -1){
			printf("Error while quitting\n");
			return -1;
	}

	close(controlSocket);
	close(dataSocket);

	free(url);

	return 0;
}

int connectFTP(){
	if((controlSocket = connectSocket(url->ip, url->port)) == 0) {
			printf("Control connection failed\n");
			return -1;
	}

	dataSocket = 0;

	if(readFTP(controlSocket, READY) == -1){
			printf("Error while connecting to server\n");
			return -1;
	}

	return 0;
}

int loginFTP(){

	sprintf(string, "USER %s\n", url->user);

	if(sendFTP(controlSocket))
			return -1;

	if(readFTP(controlSocket, SPECIFY_PASSWORD) == -1){
			printf("Error while sending username\n");
			return -1;
	}

	sprintf(string, "PASS %s\n", url->password);

	if(sendFTP(controlSocket))
			return -1;

	if(readFTP(controlSocket, LOGIN_SUCCESSFUL) == -1){
			printf("Error while sending password\n");
			return -1;
	}

	return 0;

}

int sendFTP(int fd){
	int info;

	if ((info = write(fd, string, strlen(string))) <= 0) {
		printf("There was no info sent.\n");
		return -1;
	}

	return 0;
}

int readFTP(int fd, char * code) {
	FILE* fp = fdopen(fd, "r");

	do {
		fgets(string, sizeof(string), fp);
		printf("%s", string);
	} while (!('1' <= string[0] && string[0] <= '5') || string[3] != ' ');

	if(strncmp(code, string, strlen(code)) != 0)
		return -1;


	return 0;
}

int retrieveFTP(){
	sprintf(string, "RETR %s\n", url->path);

	sendFTP(controlSocket);

	if(readFTP(controlSocket, BINARY_MODE) == -1){
			printf("Error while retrieving file\n");
			return -1;
	}

	return 0;
}

int download(){
	FILE * output;
	output = fopen(url->filename, "w");

	char buf[32768];
	int data;

	while((data = read(dataSocket, buf, sizeof(buf))) != 0){
		fwrite(buf, data, 1, output);
	}

	if(readFTP(controlSocket, TRANSFER_SUCCESSFUL) == -1){
			printf("Error while transfering file\n");
			return -1;
	}

	fclose(output);

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

int enterPassiveMode(){
	sprintf(string, "PASV\n");

	sendFTP(controlSocket);

	if(readFTP(controlSocket, PASSIVE_MODE) == -1){
			printf("Error while setting passive mode\n");
			return -1;
	}

	int ip[6];
	sscanf(string, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip[0], &ip[1], &ip[2], &ip[3], &ip[4], &ip[5]);
	sprintf(url->ip, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	url->port = ip[4]*256 + ip[5];

	if((dataSocket = connectSocket(url->ip, url->port)) == 0) {
			printf("Data connection failed\n");
			return -1;
	}


	return 0;
}
