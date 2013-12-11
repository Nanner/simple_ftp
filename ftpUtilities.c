#include "ftpUtilities.h"

struct hostent* getHostInfo(char* hostname) {
	struct hostent *h;

	if ((h=gethostbyname(hostname)) == NULL) {  
		herror("gethostbyname");
		exit(1);
	}

	return h;
}

int openTCPandConnectServer(char* hostname, unsigned int port) {
	//Get hostent struct for the hostname provided and retrieve the host address
	struct hostent* h = getHostInfo(hostname);
	char* host = inet_ntoa(*((struct in_addr *)h->h_addr));

	int	sockfd;
	struct	sockaddr_in server_addr;
	
	//Server address handling
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(host);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);		        /*server TCP port must be network byte ordered */

	//Open a TCP socket
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		perror("socket()");
		return -1;
	}

	//Connect to FTP server
	if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("connect()");
		return -1;
	}

	return sockfd;
}

int openDataPortAndDownloadFile(char* hostname, int receivedPort, char* filename) {
	pid_t  pid;
	pid = fork();

	if (pid == -1) {
		fprintf(stderr, "Can't fork, error %d\n", errno);
		exit(EXIT_FAILURE);
	}
	else if (pid == 0) {
		int	dataSocketFD = openTCPandConnectServer(hostname, receivedPort);
		if(receiveFile(dataSocketFD, filename) != 0) {
			fprintf(stderr, "Failed to receive file!\n");
			close(dataSocketFD);
			exit(1);
		}
		close(dataSocketFD);
		exit(0);
	}	
	else {
		return 0;
	}
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
	printf("<%s", response);
	return returnCode;
}

int communicate(int sockfd, char* com) {
	int bytes = send(sockfd, com, strlen(com), 0);
	printf(">%s", com);
	if(bytes != strlen(com)) {
		fprintf(stderr, "Error, problem communicating with server\n");
		return -1;
	}
	return 0;
}

int checkIfServerReady(int commandSocketFD) {
	char response[MAX_SIZE];
	int code = 0;
	code = getResponse(commandSocketFD, response);
	if(code != 220) {
		return -1;
	}

	return 0;
}

int setUsername(int commandSocketFD, char* username) {
	char response[MAX_SIZE];
	char login[MAX_SIZE];
	int code = 0;

	sprintf(login, "user %s\n", username);

	communicate(commandSocketFD, login);
	code = getResponse(commandSocketFD, response);
	if(code != 331) {
		return -1;
	}

	return 0;
}

int setPassword(int commandSocketFD, char* password) {
	char response[MAX_SIZE];
	char pass[MAX_SIZE];
	int code = 0;

	sprintf(pass, "pass %s\n", password);

	communicate(commandSocketFD, pass);
	code = getResponse(commandSocketFD, response);
	if(code != 230) {
		return -1;
	}

	return 0;
}

int setPassiveMode(int commandSocketFD) {
	char response[MAX_SIZE];
	int code = 0;

	communicate(commandSocketFD, "pasv\n");
	code = getResponse(commandSocketFD, response);
	if(code != 227) {
		return -1;
	}

	int receivedPort = 0;
	if(calculatePasvPort(&receivedPort, response) != 0) {
		return -1;
	}

	return receivedPort;
}

int retrieveFile(int commandSocketFD, char* fileUrl) {
	char response[MAX_SIZE];
	char command[MAX_SIZE];
	int code = 0;
	sprintf(command, "retr %s\n", fileUrl);

	communicate(commandSocketFD, command);
	code = getResponse(commandSocketFD, response);

	if(code != 150) {
		return -1;
	}

	return 0;
}

int receiveFile(int sockfd, char* filename) {
	FILE* f=fopen(filename, "wb");
	if(f == NULL) {
		perror("receiveFile()");
	}
	char buf[1024];
	int read;
	printf("Receiving file...\n");
	while ((read = recv(sockfd, buf, 1024, 0))) {
		fwrite(buf, read, 1, f);
	}
	printf("File received!\n");

	fclose(f);
	return 0;
}