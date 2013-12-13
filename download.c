#include "ftpUtilities.h"

#define SERVER_PORT 21

int main(int argc, char** argv){
	if (argc < 2 && argc > 3) {  
		fprintf(stderr,"Usage: download ftp://[<user>:<password>@]<host>/<url-path>\n");
		exit(1);
	}

	int verbose = 0;
	int opt = getopt (argc, argv, "v");
	if(opt == 'v') {
		verbose = 1;
	}

	printf("\n------------------------------\n");
	printf("--RCOM 2013/2014 FTP Client --\n");
	printf("Diogo Santos & Pedro Fernandes\n");
	printf("------------------------------\n\n");

	printf("Loading & connecting...\n\n");

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

	if(checkIfServerReady(commandSocketFD, verbose) != 0) {
		fprintf(stderr, "Server failed to get ready for new user\n");
		close(commandSocketFD);
		exit(1);
	}

	if(hasUserPass) {
		if(setUsername(commandSocketFD, username, verbose) != 0) {
			fprintf(stderr, "Failed to set username\n");
			close(commandSocketFD);
			exit(1);
		}

		if(setPassword(commandSocketFD, password, verbose) != 0) {
			fprintf(stderr, "Log in failed. Wrong password?\n");
			close(commandSocketFD);
			exit(1);
		}
	}
	else {
		if(setUsername(commandSocketFD, "anonymous", verbose) != 0) {
			fprintf(stderr, "Failed to set username\n");
			close(commandSocketFD);
			exit(1);
		}

		if(setPassword(commandSocketFD, "", verbose) != 0) {
			fprintf(stderr, "Log in failed. Wrong password?\n");
			close(commandSocketFD);
			exit(1);
		}
	}

	int receivedPort = setPassiveMode(commandSocketFD, verbose);
	if(receivedPort == -1) {
		fprintf(stderr, "Failed to set passive mode\n");
		close(commandSocketFD);
		exit(1);
	}

	int dataSocketFD = openDataPort(hostname, receivedPort);
	if(dataSocketFD == -1) {
		fprintf(stderr, "Failed opening the data port and downloading file from server!\n");
		close(commandSocketFD);
		exit(1);
	}

	int fileSize;
	if((fileSize = retrieveFile(commandSocketFD, fileUrl, verbose)) == -1) {
		fprintf(stderr, "Failed to open data connection. Is file OK?\n");
		close(commandSocketFD);
		exit(1);
	}

	if(downloadFile(dataSocketFD, filename, fileSize) != 0) {
		fprintf(stderr, "Failed to receive file!\n");
		close(dataSocketFD);
	}

	close(dataSocketFD);
	close(commandSocketFD);
	exit(0);
}