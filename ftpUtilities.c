#include "ftpUtilities.h"

void handler() {
  //Kill child process, if created
  if (child > 0) {
    kill (child, SIGINT);
    waitpid (child, 0, 0);
  }

  //Kill self
  kill (getpid (), SIGTERM);
}
struct hostent* getHostInfo(char* hostname) {
	struct hostent *h;

	if ((h=gethostbyname(hostname)) == NULL) {
		return NULL; 
	}

	printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n",inet_ntoa(*((struct in_addr *)h->h_addr)));

	return h;
}

int openTCPandConnectServer(char* hostname, unsigned int port) {
	//Get hostent struct for the hostname provided and retrieve the host address
	struct hostent* h = getHostInfo(hostname);
	if(h == NULL) {
		printf("Could not resolve hostname\n");
		return -1;
	}
	char* host = inet_ntoa(*((struct in_addr *)h->h_addr));

	/*
	int	sockfd;
	struct	sockaddr_in server_addr;
	
	//Server address handling
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(host);	//32 bit Internet address network byte ordered
	server_addr.sin_port = htons(port);		        //server TCP port must be network byte ordered

	//Open a TCP socket
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		perror("socket()");
		return -1;
	}

	printf("%s\n", host);
	printf("%u\n", server_addr.sin_addr.s_addr);

	//Connect to FTP server
	if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("connect()");
		return -1;
	}

	printf("Lol\n");	

	return sockfd;
	*/

	struct addrinfo server_addr, *res;
	int sockfd, status;

	// first, load up address structs with getaddrinfo():

	memset(&server_addr, 0, sizeof server_addr);
	server_addr.ai_family = AF_INET;
	server_addr.ai_socktype = SOCK_STREAM;

	char* portStr = itoa(port);
	printf("%s\n", portStr);
	if((status = getaddrinfo(host, portStr, &server_addr, &res)) != 0) {
	    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
	    exit(1);
	}

	// make a socket:
	if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
		perror("socket()");
		return -1;
	}

	// connect!
	if((connect(sockfd, res->ai_addr, res->ai_addrlen) < 0)) {
		perror("connect()");
		return -1;
	}
	printf("Opened socket on port %s\n", portStr);
	return sockfd;
}

int openDataPort(char* hostname, int receivedPort) {
	int	dataSocketFD = openTCPandConnectServer(hostname, receivedPort);
	return(dataSocketFD);
}

int downloadFile(int dataSocketFD, char* filename) {
	if(receiveFile(dataSocketFD, filename) != 0) {
		return(-1);
	}
	return 0;
}

int getResponse(int sockfd, char* response, int fullResponse) {
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
	printf("hey.\n");
	int returnCode = getReturnCode(response);
	//printf("<%s", response);
	return returnCode;
	/*char buf[1];
	int read, totalRead = 0;
	char code[3];
	int i = 0;
	while (1) {
		read = recv(sockfd, buf, 1, MSG_DONTWAIT);
		
        if(errno == EAGAIN) {
        	if(totalRead == 0)
        		continue;
        	else if(totalRead > 0)
        		break;
        }
		if(read <= 0)
			break;
		if(i < 3) {
			code[i] = buf[0];
			i++;
		}
		else {
			if(clean)
				break;
		}
		fwrite(buf, read, 1, stdout);
		totalRead += read;
		printf("printing\n");
		//printf("%d\n", buf[0]);
	}
	fwrite("\n\0", 2, 1, stdout);
	if(clean)
		cleanUpSocket(sockfd);
	//printf("File received!\n");

	//fclose(f);
	//printf("%d\n", atoi(code));
	return atoi(code);
	*/
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
	code = getResponse(commandSocketFD, response, 1);
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
	code = getResponse(commandSocketFD, response, 1);
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
	code = getResponse(commandSocketFD, response, 1);
	if(code != 230) {
		return -1;
	}

	return 0;
}

int setPassiveMode(int commandSocketFD) {
	char response[MAX_SIZE];
	int code = 0;

	communicate(commandSocketFD, "pasv\n");
	code = getResponse(commandSocketFD, response, 0);
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

	printf("comunicated, going to get response\n");
	code = getResponse(commandSocketFD, response, 1);

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

int cleanUpSocket(int socketfd) {
    int r;
    char buf[1024];
    do {
        r = recv(socketfd, buf, 1024, MSG_DONTWAIT);
        if (r < 0 && errno == EINTR)
        	continue;
    }
    while (r > 0);

    if (r < 0 && errno != EWOULDBLOCK) {
        perror(__func__);
        return -1;
    }
    return r;
}