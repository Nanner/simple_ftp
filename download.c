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

	//Get the hostname and the full url of the file to be downloaded via ftp
	if(parseHostnameAndUrl(hostname, fileUrl, argv[1]) != 0) {
		fprintf(stderr, "Failed to parse Hostname and file URL\n");
		exit(1);
	}

	//Get the filename
	filename = parseFilename(fileUrl);

	//Open the TCP socket and connect to the ftp server
	int commandSocketFD = openTCPandConnectServer(hostname, SERVER_PORT);
	if(commandSocketFD == -1) {
		exit(0);
	}

	if(checkIfServerReady(commandSocketFD) != 0) {
		fprintf(stderr, "Server failed to get ready for new user\n");
		close(commandSocketFD);
		exit(1);
	}

	if(hasUserPass) {
		if(setUsername(commandSocketFD, username) != 0) {
			fprintf(stderr, "Failed to set username\n");
			close(commandSocketFD);
			exit(1);
		}

		if(setPassword(commandSocketFD, password) != 0) {
			fprintf(stderr, "Log in failed. Wrong password?\n");
			close(commandSocketFD);
			exit(1);
		}
	}
	else {
		if(setUsername(commandSocketFD, "anonymous") != 0) {
			fprintf(stderr, "Failed to set username\n");
			close(commandSocketFD);
			exit(1);
		}

		if(setPassword(commandSocketFD, "") != 0) {
			fprintf(stderr, "Log in failed. Wrong password?\n");
			close(commandSocketFD);
			exit(1);
		}
	}

	int receivedPort = setPassiveMode(commandSocketFD);
	if(receivedPort == -1) {
		fprintf(stderr, "Failed to set passive mode\n");
		close(commandSocketFD);
		exit(1);
	}

	if(openDataPortAndDownloadFile(hostname, receivedPort, filename) != 0) {
		fprintf(stderr, "Failed opening the data port and downloading file from server!\n");
		close(commandSocketFD);
		exit(1);
	}

	usleep(1000);
	if(retrieveFile(commandSocketFD, fileUrl) != 0) {
		fprintf(stderr, "Failed to open data connection. Is file OK?\n");
		close(commandSocketFD);
		exit(1);
	}

	close(commandSocketFD);
	exit(0);
}