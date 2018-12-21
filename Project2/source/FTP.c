#include "FTP.h"

URL * url;
int controlSocket;
int dataSocket;
char string[COMMAND_SIZE];
char returnCode[COMMAND_SIZE];
char returnMessage[COMMAND_SIZE];

int main(int argc, char** argv){
	if (argc != 2) {
			printf("%s000%s Please insert a valid url: './download ftp://[<user>:<password>@]<host>/<url-path>'\n", RED, RESET);
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

	printf("%s000%s IP: %s\n", BLUE, RESET, url->ip);
	printf("%s000%s Port: %d\n", BLUE, RESET, url->port);

	if((dataSocket = connectSocket(url->ip, url->port)) == 0)
			return -1;


	return 0;
}

int filesize(){
	sprintf(string, "SIZE %s\n", url->path);

	if(ftpSend(controlSocket) == -1)
			return -1;

	if(ftpRead(controlSocket, FILESIZE, FALSE) == -1){
			printf("%s%s%s %s\n", RED, "550", RESET, "File not found");
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

	printf("%s000%s Transfering %s\n", BLUE, RESET, url->filename);

	while((data = read(dataSocket, buf, sizeof(buf))) != 0){
		fwrite(buf, data, 1, output);
		size += data;
		progress(url, size);
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
			if(print){
					char digit1, digit2, digit3;
					if(sscanf(string, "%c%c%c-%[^\t\n]", &digit1, &digit2, &digit3, returnMessage) != 4)
						sscanf(string, "%c%c%c %[^\t\n]", &digit1, &digit2, &digit3, returnMessage);
					sprintf(returnCode, "%c%c%c", digit1, digit2, digit3);
					printf("%s%s%s %s\n", GREEN, returnCode, RESET, returnMessage);
			}
	} while (!(string[0] >= '1' && string[0] <= '5') || string[3] != ' ');

	if(strncmp(code, string, strlen(code)) != 0)
			return -1;

	return 0;
}
