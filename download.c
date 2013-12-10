/*      (C)2000 FEUP  */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>

#include <regex.h> //to parse the argument

//#define FTP_URL_REGEX "^(ftp)://([a-zA-Z0-9.-]+(:[a-zA-Z0-9.&amp;%%-]+)*@)*((25[0-5]|2[0-4][0-9]|[0-1]{1}[0-9]{2}|[1-9]{1}[0-9]{1}|[1-9]).(25[0-5]|2[0-4][0-9]|[0-1]{1}[0-9]{2}|[1-9]{1}[0-9]{1}|[1-9]|0).(25[0-5]|2[0-4][0-9]|[0-1]{1}[0-9]{2}|[1-9]{1}[0-9]{1}|[1-9]|0).(25[0-5]|2[0-4][0-9]|[0-1]{1}[0-9]{2}|[1-9]{1}[0-9]{1}|[0-9])|localhost|([a-zA-Z0-9-]+.)*[a-zA-Z0-9-]+.(com|edu|gov|int|mil|net|org|biz|arpa|info|name|pro|aero|coop|museum|[a-zA-Z]{2}))(:[0-9]+)*(/(|[a-zA-Z0-9.,\?\'\\+&amp;%%#=~_-]+))*$"
#define SERVER_PORT 21
#define SERVER_ADDR "192.168.28.96"
#define MAX_SIZE 255

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
	printf("<%s\n", response);
	return respRes;
}

int comunicate(int sockfd, char* com) {
	int bytes = send(sockfd, com, strlen(com), 0);
	printf(">%s\n", com);
	if(bytes != strlen(com)) {
		fprintf(stderr, "Error, sent %d bytes when size was %u bytes\n", bytes, strlen(com));
		return -1;
	}
	return 0;
}

/*int isValidUrl(char* ftpURL) {
	regex_t regex;
	char msgbuf[100];

	//Compile regular expression
	int ret = regcomp(&regex, FTP_URL_REGEX, 0);
	if(ret) {
		fprintf(stderr, "Could not compile regex\n");
		exit(1);
	}

	//Test the ftp url against the regular expression
	ret = regexec(&regex, ftpURL, 0, NULL, 0);
	if(!ret) {
		return 0;
	}
	else if(ret == REG_NOMATCH) {
		return -1;
	}
	else{
		regerror(ret, &regex, msgbuf, sizeof(msgbuf));
		fprintf(stderr, "Regex match failed: %s\n", msgbuf);
		exit(1);
	}
}*/

struct hostent* getHostInfo(char* hostname) {
	struct hostent *h;


/*
struct hostent {
	char    *h_name;	Official name of the host. 
    	char    **h_aliases;	A NULL-terminated array of alternate names for the host. 
	int     h_addrtype;	The type of address being returned; usually AF_INET.
    	int     h_length;	The length of the address in bytes.
	char    **h_addr_list;	A zero-terminated array of network addresses for the host. 
				Host addresses are in Network Byte Order. 
};

#define h_addr h_addr_list[0]	The first address in h_addr_list. 
*/
	if ((h=gethostbyname(hostname)) == NULL) {  
		herror("gethostbyname");
		exit(1);
	}

	printf("Host name  : %s\n", h->h_name);
	printf("IP Address : %s\n",inet_ntoa(*((struct in_addr *)h->h_addr)));

	return h;
}

int main(int argc, char** argv){

	if (argc != 2) {  
		fprintf(stderr,"Usage: download ftp://[<user>:<password>@]<host>/<url-path>\n");
		exit(1);
	}

	/*if(isValidUrl(argv[1]) == 0) {
		printf("Valid url!\n");
	}
	else
		printf("Invalid url!\n");*/

		unsigned int urlSize = strlen(argv[1]);
	char* atPtr = strchr(argv[1], '@');
	int hasUserPass = 0;
	if(atPtr != NULL) {
		hasUserPass = 1;
	}
	char username[MAX_SIZE] = "";
	char password[MAX_SIZE] = "";
	char hostname[MAX_SIZE] = "";
	char fileUrl[MAX_SIZE] = "";
	//Parse username and password
	if(hasUserPass) {
		//Try to find the : between user and password
		char* colonMatch = strchr(argv[1] + 6, ':');
		if(colonMatch == NULL) {
			fprintf(stderr, "Invalid url!");
			exit(1);
		}

		 //Calculate distance from beggining of string
		size_t uslen = colonMatch - argv[1];
		if(uslen >= (urlSize - 1)) {
			uslen = (urlSize - 1);
		}
		 //Ignore the ftp:// part
		uslen -= 6;
		memcpy(username, argv[1] + 6, uslen);
		username[uslen] = '\0';
		printf("%s\n", username);

		 //Try to find the @ after password
		char* atMatch = strchr(argv[1] + 6 + strlen(username), '@');
		if(atMatch == NULL) {
			fprintf(stderr, "Invalid url!");
			exit(1);
		}

		 //Calculate distance from beggining of string
		size_t paslen = atMatch - argv[1];
		if(paslen >= (urlSize - 1)) {
			paslen = (urlSize - 1);
		}
		 //Ignore the ftp://username part
		paslen -= (6 + strlen(username) + 1);
		memcpy(password, argv[1] + 6 + strlen(username) + 1, paslen);
		password[paslen] = '\0';
		printf("%s\n", password);
	}

	//Find hostname position in url string
	size_t hostPosition = 6;
	if(hasUserPass) {
		hostPosition += (2 + strlen(username) + strlen(password));
	}

	//Try to find the first /
	char* slashMatch = strchr(argv[1] + 6, '/');
	if(slashMatch == NULL) {
		fprintf(stderr, "Invalid url!");
		exit(1);
	}

	//Calculate distance from beggining of string
	size_t slashlen = slashMatch - argv[1];
	if(slashlen >= (urlSize - 1)) {
		slashlen = (urlSize - 1);
	}
	
	//Ignore the ftp:// part
	slashlen -= hostPosition;
	memcpy(hostname, argv[1] + hostPosition, slashlen);
	hostname[slashlen] = '\0';
	printf("%s\n", hostname);

	memcpy(fileUrl, argv[1] + hostPosition, urlSize - hostPosition);
	printf("%s\n", fileUrl);

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
getResponse(sockfd, response);

comunicate(sockfd, "helo\n");

getResponse(sockfd, response);

char login[MAX_SIZE];
sprintf(login, "user %s\n", username);
comunicate(sockfd, login);

getResponse(sockfd, response);

char pass[MAX_SIZE];
sprintf(pass, "pass %s\n", password);
comunicate(sockfd, pass);

getResponse(sockfd, response);

comunicate(sockfd, "pasv\n");

getResponse(sockfd, response);
char rawConNumbers[MAX_SIZE];
 //Try to find the @ after password
char* parMatch = strchr(response, '(');
if(parMatch == NULL) {
	fprintf(stderr, "Invalid passive mode response!\n");
	exit(1);
}

//Calculate distance from beggining of string
size_t rawLen = parMatch - response;
size_t responseLen = strlen(response);
if(rawLen >= responseLen - 1) {
	rawLen = responseLen - 1;
}

size_t rawConNumbersLen = responseLen - rawLen - 5;
memcpy(rawConNumbers, response + rawLen + 1, rawConNumbersLen);
rawConNumbers[rawConNumbersLen] = '\0';
printf("%s\n", rawConNumbers);


int i;
char *p;
int numbers[6];
i = 0;
p = strtok (rawConNumbers,",");  
while (p != NULL)
{
  numbers[i++] = atoi(p);
  p = strtok (NULL, ",");
}
for (i=0;i<6; ++i) 
  printf("%d\n", numbers[i]);

int receivedPort = numbers[4] * 256 + numbers[5];
printf("Port: %d\n", receivedPort);

	int	sockfd2;
	struct	sockaddr_in server_addr2;
	
	/*server address handling*/
	bzero((char*)&server_addr2,sizeof(server_addr2));
	server_addr2.sin_family = AF_INET;
	server_addr2.sin_addr.s_addr = inet_addr(host);	/*32 bit Internet address network byte ordered*/
	server_addr2.sin_port = htons(receivedPort);		/*server TCP port must be network byte ordered */

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

getResponse(sockfd2, response);

close(sockfd);
close(sockfd2);
exit(0);
}

