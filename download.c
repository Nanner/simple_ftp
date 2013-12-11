#include "ftpUtilities.h"

#define SERVER_PORT 21

int main(int argc, char** argv){

	if (argc != 2) {  
		fprintf(stderr,"Usage: download ftp://[<user>:<password>@]<host>/<url-path>\n");
		exit(1);
	}

	char username[MAX_SIZE] = "";
	char password[MAX_SIZE] = "";
	char hostname[MAX_SIZE] = "";
	char fileUrl[MAX_SIZE] = "";
	char* filename = NULL;

	//Check if it has a [user:pass@] field
	int hasUserPass = hasLoginField(argv[1]);

	//Parse username and password, in case it has a [user:pass@] field
	if(hasUserPass) {
		if(parseUsername(username, argv[1]) != 0) {
			fprintf(stderr, "Failed to parse Username\n");
			exit(1);
		}

		if(parsePassword(password, argv[1]) != 0) {
			fprintf(stderr, "Failed to parse Password\n");
			exit(1);
		}
	}

	if(parseHostnameAndUrl(hostname, fileUrl, argv[1]) != 0) {
		fprintf(stderr, "Failed to parse Hostname and file URL\n");
		exit(1);
	}

	filename = parseFilename(fileUrl);

	struct hostent* h = getHostInfo(hostname);
	char* host = inet_ntoa(*((struct in_addr *)h->h_addr));

	int	sockfd;
	struct	sockaddr_in server_addr;
	
	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(host);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */

	/*open a TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		perror("socket()");
		exit(0);
	}
	/*connect to the server*/
	if(connect(sockfd, 
		(struct sockaddr *)&server_addr, 
		sizeof(server_addr)) < 0){
		perror("connect()");
		exit(0);
	}


	char response[MAX_SIZE];
	int code;
	code = getResponse(sockfd, response);
	if(code != 220) {
		fprintf(stderr, "Server failed to get ready for new user\n");
		exit(1);
	}

	char login[MAX_SIZE];
	sprintf(login, "user %s\n", username);

	communicate(sockfd, login);
	code = getResponse(sockfd, response);
	if(code != 331) {
		fprintf(stderr, "Failed to set username\n");
		exit(1);
	}

	char pass[MAX_SIZE];
	sprintf(pass, "pass %s\n", password);

	communicate(sockfd, pass);
	code = getResponse(sockfd, response);
	if(code != 230) {
		fprintf(stderr, "Log in failed. Wrong password?\n");
		exit(1);
	}

	communicate(sockfd, "pasv\n");
	code = getResponse(sockfd, response);
	if(code != 227) {
		fprintf(stderr, "Failed to set passive mode\n");
		exit(1);
	}

	int receivedPort = 0;
	if(calculatePasvPort(&receivedPort, response) != 0) {
		fprintf(stderr, "Failed to set passive mode\n");
		exit(1);
	}

	printf("Port: %d\n", receivedPort);

	openDataPort(host, receivedPort, filename);

	usleep(1000);
	char command[MAX_SIZE];
	sprintf(command, "retr %s\n", fileUrl);

	communicate(sockfd, command);
	code = getResponse(sockfd, response);

	if(code != 150) {
		fprintf(stderr, "Failed to open data connection. Is file OK?\n");
		exit(1);
	}

	close(sockfd);
	exit(0);
	while(1);
}