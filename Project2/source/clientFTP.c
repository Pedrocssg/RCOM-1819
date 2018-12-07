#include "clientFTP.h"

URL * url;
int socketfd;

int main(int argc, char** argv){
	if (argc != 2) {
			fprintf(stderr,"usage: getip address\n");
			exit(1);
	}

	url = malloc(sizeof(URL));
	url->port = FTP_PORT;

	if(sscanf(argv[1], "ftp://%99[^:]:%99[^@]@%99[^/]/%99[^\n]", url->user, url->password, url->host, url->path) != 4) {
			printf("Error while parsing URL. Please enter in this format 'ftp://user:password@host/filepath'\n");
			exit(1);
	}

	getIp(url->ip, url->host);

	if((socketfd = connectSocket()) == 0) {
			printf("Control connection failed\n");
			exit(1);
	}

	char	buf[] = "Mensagem\n";
	int	bytes;

	/*send a string to the server*/
	bytes = write(socketfd, buf, strlen(buf));
	printf("Bytes escritos %d\n", bytes);

	close(socketfd);
	exit(0);
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
