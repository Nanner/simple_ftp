#include "ftpUtilities.h"

int receiveFile(int sockfd, char* filename) {
	printf("%s\n", "filename");
	FILE* f=fopen(filename, "wb");
	if(f == NULL) {
		perror("receiveFile()");
	}
	char buf[1024];
	int read;
	printf("Receiving file...\n");
	while ((read = recv(sockfd, buf, 1024, 0))) {
        //printf("Read %d bytes", read);
		fwrite(buf, read, 1, f);
        //printf("Writing %d\n", read);
	}
	printf("File received!\n");

	fclose(f);
	return 0;
}

int getResponse(int sockfd, char* response) {
	int respRes = recv(sockfd, response, MAX_SIZE, 0);
	if(respRes == 0) {
		fprintf(stderr, "Host closed connection!\n");
		exit(1);
	}
	if(respRes == -1) {
		perror("recv()");
		exit(1);
	}
	response[respRes] = '\0';
	int returnCode = getReturnCode(response);
	printf("<%s\n", response);
	printf("Code: %d\n", returnCode);
	return returnCode;
}

int communicate(int sockfd, char* com) {
	int bytes = send(sockfd, com, strlen(com), 0);
	printf(">%s\n", com);
	if(bytes != strlen(com)) {
		fprintf(stderr, "Error, problem communicating with server\n");
		return -1;
	}
	return 0;
}

int openDataPort(char* host, int receivedPort, char* filename) {
	pid_t  pid;
	pid = fork();

	if (pid == -1) {
		fprintf(stderr, "can't fork, error %d\n", errno);
		exit(EXIT_FAILURE);
	}
	else if (pid == 0) {
		int	sockfd2;
		struct	sockaddr_in server_addr2;

	/*server address handling*/
	bzero((char*)&server_addr2,sizeof(server_addr2));
	server_addr2.sin_family = AF_INET;
	server_addr2.sin_addr.s_addr = inet_addr(host);	/*32 bit Internet address network byte ordered*/
	server_addr2.sin_port = htons(receivedPort);	/*server TCP port must be network byte ordered*/

	/*open a TCP socket*/
		if ((sockfd2 = socket(AF_INET,SOCK_STREAM,0)) < 0) {
			perror("socket()");
			exit(0);
		}
	/*connect to the server*/
		if(connect(sockfd2, 
			(struct sockaddr *)&server_addr2, 
			sizeof(server_addr2)) < 0){
			perror("connect()");
		exit(0);
	}

	if(receiveFile(sockfd2, filename) != 0) {
		fprintf(stderr, "Failed to receive file!\n");
		exit(1);
	}
	close(sockfd2);
	exit(0);

}
else
{
	return 0;
}
return 0;
}

struct hostent* getHostInfo(char* hostname) {
	struct hostent *h;

	if ((h=gethostbyname(hostname)) == NULL) {  
		herror("gethostbyname");
		exit(1);
	}

	printf("Host name  : %s\n", h->h_name);
	printf("IP Address : %s\n",inet_ntoa(*((struct in_addr *)h->h_addr)));

	return h;
}