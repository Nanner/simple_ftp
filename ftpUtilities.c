#include "ftpUtilities.h"

// loadBar code based on
// http://www.rosshemsley.co.uk/2011/02/creating-a-progress-bar-in-c-or-any-other-console-app/
// Process has done x out of n rounds,
// and we want a bar of width w and resolution r.
static inline void loadBar(int x, int n, int r, int w, int doneSize, int totalSize) {
    // Only update r times.
    if ( x % (n/r) != 0 ) return;
 
    // Calculuate the ratio of complete-to-incomplete.
    float ratio = x/(float)n;
    int   c     = ratio * w;
 
    // Show the percentage complete.
    printf("%3d%%[", (int)(ratio*100));
 
    // Show the load bar.
    int y;
    for (y = 0; y < c; y++)
       printf("=");
 
    for (y = c; y < w; y++)
       printf(" ");
 
    // ANSI Control codes to go back to the
    // previous line and clear it.
   	printf("] %d/%d B", doneSize, totalSize);
	printf("\r"); // Move to the first column
	fflush(stdout);
	if(x == n) {
		printf("\n");
	}
}

struct hostent* getHostInfo(char* hostname) {
	struct hostent *h;

	if ((h=gethostbyname(hostname)) == NULL) {
		return NULL; 
	}

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

	struct addrinfo server_addr, *res;
	int sockfd, status;

	// first, load up address structs with getaddrinfo():
	memset(&server_addr, 0, sizeof server_addr);
	server_addr.ai_family = AF_INET;
	server_addr.ai_socktype = SOCK_STREAM;

	char* portStr = itoa(port);
	if((status = getaddrinfo(host, portStr, &server_addr, &res)) != 0) {
	    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
	    exit(1);
	}

	//create socket
	if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
		perror("socket()");
		return -1;
	}

	// connect
	if((connect(sockfd, res->ai_addr, res->ai_addrlen) < 0)) {
		perror("connect()");
		return -1;
	}

	return sockfd;
}

int openDataPort(char* hostname, int receivedPort) {
	int	dataSocketFD = openTCPandConnectServer(hostname, receivedPort);
	return(dataSocketFD);
}

int getResponse(int sockfd, char* response, int verbose) {
	int ind = 0, totalSize = 0;
	char line[MAX_LINE_SIZE];
	int isMultiLine = 0;
	int returnCode = 0;

	//Read a line
	ind = readLine(sockfd, line);
	//Copy said line to the response variable
	memcpy(response+totalSize, line, ind);
	//Calculate the line "return code"
	returnCode = getReturnCode(line);
	//Update total response size with line size
	totalSize += ind;  
	//Get line identifier(i.e.: "220-" or "220 ")                      
	char ident[4];
	getLineIdentifier(line, ident);

	//A multiline FTP response is identified by
	//[code]-
	//[code]- something
	//[code]- something
	//[code]- something
	//[code]
	//So we need to read from the server while we dont get a return code the same as the first line's followed
	//by an empty space
	if(ident[3] == '-')
		isMultiLine = 1;
	if(isMultiLine) {
		int lineReturnCode;
		char lineIdent[4];
		do {
			ind = readLine(sockfd, line);
			memcpy(response+totalSize, line, ind);
			lineReturnCode = getReturnCode(line);
			totalSize += ind;
			getLineIdentifier(line, lineIdent);
		} while(lineReturnCode != returnCode || lineIdent[3] != ' ');
	}

	response[totalSize] = '\0';

	if(verbose)
		printf("%s", response);

	return returnCode;
}

int readLine(int sockfd, char* line) {
	int lineSize = 0;
	char buf[1];
	int r = 0;

	//An FTP line always ends on a LF+CR sequence, that is, line feed (newline) and carriage return
	//So we keep on reading until we find this sequence
	do {
		r = recv(sockfd, buf, 1, 0);
		if(r == 0) {
			fprintf(stderr, "Host closed connection!\n");
			exit(1);
		}
		if(r == -1) {
			perror("recv()");
			exit(1);
		}
		line[lineSize] = buf[0];
		lineSize++;
	}
	while(lineSize < 2 || line[lineSize - 1] != '\n' || line[lineSize - 2] != '\r');
	return lineSize;
}

int communicate(int sockfd, char* com, int verbose) {
	int bytes = send(sockfd, com, strlen(com), 0);

	if(bytes != strlen(com)) {
		fprintf(stderr, "Error, problem communicating with server\n");
		return -1;
	}

	if(verbose)
		printf(">%s", com);

	return 0;
}

int checkIfServerReady(int commandSocketFD, int verbose) {
	char response[MAX_SIZE];
	int code = 0;
	code = getResponse(commandSocketFD, response, verbose);
	if(code != 220) {
		printf("Failed with code %d\n", code);
		return -1;
	}

	return 0;
}

int setUsername(int commandSocketFD, char* username, int verbose) {
	char response[MAX_SIZE];
	char login[MAX_SIZE];
	int code = 0;

	sprintf(login, "user %s\n", username);

	communicate(commandSocketFD, login, verbose);

	code = getResponse(commandSocketFD, response, verbose);

	if(code != 331) {
		printf("Failed with code %d\n", code);
		return -1;
	}

	return 0;
}

int setPassword(int commandSocketFD, char* password, int verbose) {
	char response[MAX_SIZE];
	char pass[MAX_SIZE];
	int code = 0;

	sprintf(pass, "pass %s\n", password);

	communicate(commandSocketFD, pass, verbose);

	code = getResponse(commandSocketFD, response, verbose);

	if(code != 230) {
		printf("Failed with code %d\n", code);
		return -1;
	}

	return 0;
}

int setPassiveMode(int commandSocketFD, int verbose) {
	char response[MAX_SIZE];
	int code = 0;

	communicate(commandSocketFD, "pasv\n", verbose);

	code = getResponse(commandSocketFD, response, verbose);
	if(code != 227) {
		printf("Failed with code %d\n", code);
		return -1;
	}

	int receivedPort = 0;
	if(calculatePasvPort(&receivedPort, response) != 0) {
		return -1;
	}

	return receivedPort;
}

int retrieveFile(int commandSocketFD, char* fileUrl, int verbose) {
	char response[MAX_SIZE];
	char command[MAX_SIZE];
	int code = 0;
	sprintf(command, "retr %s\n", fileUrl);

	communicate(commandSocketFD, command, verbose);

	code = getResponse(commandSocketFD, response, verbose);
	int size = getFileSize(response);

	if(code != 150) {
		printf("Failed with code %d\n", code);
		return -1;
	}

	return size;
}

int downloadFile(int dataSocketFD, char* filename, int fileSize) {
	if(receiveFile(dataSocketFD, filename, fileSize) != 0) {
		return(-1);
	}
	return 0;
}

int receiveFile(int sockfd, char* filename, int fileSize) {
	FILE* f=fopen(filename, "wb");
	if(f == NULL) {
		perror("receiveFile()");
	}
	char buf[1024];
	int read;
	int sizeReceived = 0;
	printf("Receiving file...\n");
	loadBar(0, fileSize, fileSize, 50, 0, fileSize);
	while ((read = recv(sockfd, buf, 1024, 0))) {
		fwrite(buf, read, 1, f);
		sizeReceived += read;
		loadBar(sizeReceived, fileSize, fileSize, 50, sizeReceived, fileSize);
	}
	printf("File received!\n");

	fclose(f);
	return 0;
}