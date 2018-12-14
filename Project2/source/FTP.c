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

	close(controlSocket);
	exit(0);
}

int connectFTP(){
	if((controlSocket = connectSocket()) == 0) {
			printf("Control connection failed\n");
			return -1;
	}

	dataSocket = 0;

	if(readFTP(SERVER_READY) == -1){
			printf("Error while connecting to server\n");
			return -1;
	}

	return 0;
}

int loginFTP(){

	sprintf(string, "USER %s\n", url->user);

	if(sendFTP())
			return -1;

	if(readFTP(PASSSWORD_REQUIRED) == -1){
			printf("Error while sending username\n");
			return -1;
	}

	sprintf(string, "PASS %s\n", url->password);

	if(sendFTP())
			return -1;

	if(readFTP(LOGIN_SUCCESSFULL) == -1){
			printf("Error while sending password\n");
			return -1;
	}

	return 0;

}

int sendFTP() {
	int info;

	if ((info = write(controlSocket, string, strlen(string))) <= 0) {
		printf("There was no info sent.\n");
		return -1;
	}

	return 0;
}

int readFTP(char * code) {
	FILE* fp = fdopen(controlSocket, "r");

	do {
		fgets(string, sizeof(string), fp);
		printf("%s", string);
	} while (!('1' <= string[0] && string[0] <= '5') || string[3] != ' ');

	if(strncmp(code, string, strlen(code)) != 0)
		return -1;


	return 0;
}

int connectSocket() {
	int	socketfd;
	struct sockaddr_in server_addr;

	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(url->ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(url->port);		/*server TCP port must be network byte ordered */

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

	sendFTP();

	if(readFTP(PASSIVE) == -1){
			printf("Error while setting passive mode\n");
			return -1;
	}

	int ip[6];
	sscanf(string, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip[0], &ip[1], &ip[2], &ip[3], &ip[4], &ip[5]);
	sprintf(url->ip, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	url->port = ip[4]*256 + ip[5];

	printf("Port - %d\n", url->port);

	return 0;
}
