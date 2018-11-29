#include "clientTCP.h"

int main(int argc, char** argv){
	if (argc != 2) {
			fprintf(stderr,"usage: getip address\n");
			exit(1);
	}

	char user[255], password[255], host[255], url[255];
	parseFTPrequest(argv[1], user, password, host, url);

	printf("ftp://%s:%s@%s/%s\n", user, password, host, url);

	exit(0);

	char	buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";
	int	bytes;

	int socketfd = connectSocket();

	/*send a string to the server*/
	bytes = write(socketfd, buf, strlen(buf));
	printf("Bytes escritos %d\n", bytes);

	close(socketfd);
	exit(0);
}

int parseFTPrequest(char *request, char *user, char *password, char *host, char *url) {
	char *tok;
	char *delim = ":@/";

	tok = strtok(request, delim);
	if (strcmp(tok, "ftp") != 0) {
		printf("WRONG FORMAT: _://<user>:<pass>@<host>/<url-path>\n");
		return -1;
	}

	tok = strtok(request, delim);
	//printf("%s\n",tok);
	if (strcmp(tok, "") != 0) {
		printf("WRONG FORMAT: ftp:_//<user>:<pass>@<host>/<url-path>\n");
		return -1;
	}

	tok = strtok(request, delim);
	if (strcmp(tok, "") != 0) {
		printf("WRONG FORMAT: ftp:/_/<user>:<pass>@<host>/<url-path>\n");
		return -1;
	}

	tok = strtok(request, delim);
	if (tok != NULL) {
		sprintf(user, "%s", tok);
	}
	else {
		printf("WRONG FORMAT: ftp://<_>:<pass>@<host>/<url-path>\n");
		return -1;
	}

	tok = strtok(request, delim);
	if (tok != NULL) {
		sprintf(password, "%s", tok);
	}
	else {
		printf("WRONG FORMAT: ftp://<user>:<_>@<host>/<url-path>\n");
		return -1;
	}

	tok = strtok(request, delim);
	if (tok != NULL) {
		sprintf(host, "%s", tok);
	}
	else {
		printf("WRONG FORMAT: ftp://<user>:<pass>@<_>/<url-path>\n");
		return -1;
	}

	tok = strtok(request, "");
	if (tok != NULL) {
		sprintf(url, "%s", tok);
	}
	else {
		printf("WRONG FORMAT: ftp://<user>:<pass>@<host>/<_>\n");
		return -1;
	}

	return 0;
}

int connectSocket() {
	int	socketfd;
	struct sockaddr_in server_addr;

	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */

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
