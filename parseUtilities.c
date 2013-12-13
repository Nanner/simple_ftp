#include "parseUtilities.h"

int hasLoginField(char* url) {
	char* atPtr = strchr(url, '@');
	if(atPtr != NULL) {
		return 1;
	}
	else
		return 0;
}

int parseUsername(char* username, char* url) {
	//Try to find the : between user and password
	char* colonMatch = strchr(url + 6, ':');
	if(colonMatch == NULL) {
		return -1;
	}

	//Calculate distance from beggining of string
	size_t urlSize = strlen(url);
	size_t uslen = colonMatch - url;
	if(uslen >= (urlSize - 1)) {
		uslen = (urlSize - 1);
	}

	//Ignore the ftp:// part
	uslen -= 6;
	memcpy(username, url + 6, uslen);
	username[uslen] = '\0';
	return 0;
}

int parsePassword(char* password, char* url) {
	//Try to find the : between user and password
	char* colonMatch = strchr(url + 6, ':');
	if(colonMatch == NULL) {
		return -1;
	}

	//Try to find the @ after password
	char* atMatch = strchr(colonMatch + 1, '@');
	if(atMatch == NULL) {
		return -1;
	}

	//Calculate distance from beggining of string
	size_t urlSize = strlen(url);
	size_t paslen = atMatch - url;
	if(paslen >= (urlSize - 1)) {
		paslen = (urlSize - 1);
	}

	//Calculate distance from beggining of string to :
	size_t uslen = colonMatch - url;
	if(uslen >= (urlSize - 1)) {
		uslen = (urlSize - 1);
	}

	//Ignore the ftp://username: part
	paslen -= (uslen + 1);
	memcpy(password, colonMatch + 1, paslen);
	password[paslen] = '\0';

	return 0;
}

int parseHostnameAndUrl(char* hostname, char* fileUrl, char* url) {
	//Find hostname position in url string
	size_t urlSize = strlen(url);
	size_t hostPosition = 6; //right after ftp://
	char* atMatch = strchr(url, '@');
	if(atMatch != NULL) {

		size_t atlen = atMatch - url;
		if(atlen >= (urlSize - 1)) {
			atlen = (urlSize - 1);
		}
		hostPosition = atlen + 1; //right after ftp://user:pass@
	}

	//Try to find the first /
	char* slashMatch = strchr(url + 6, '/');
	if(slashMatch == NULL) {
		return -1;
	}

	//Calculate distance from beggining of string
	size_t slashlen = slashMatch - url;
	if(slashlen >= (urlSize - 1)) {
		slashlen = (urlSize - 1);
	}
	
	//Ignore the ftp://user:pass@ part
	slashlen -= hostPosition;
	memcpy(hostname, url + hostPosition, slashlen);
	hostname[slashlen] = '\0';

	memcpy(fileUrl, slashMatch + 1, urlSize - (slashlen + hostPosition));

	return 0;
}

char* parseFilename(char* fileUrl) {
	return(basename(fileUrl));
}

int getReturnCode(char* response) {
	char code[3];
	memcpy(code, response, 3);
	return atoi(code);
}

void getLineIdentifier(char* response, char* ident) {
	memcpy(ident, response, 4);
}

int calculatePasvPort(int* port, char* response) {
	char rawConNumbers[MAX_SIZE];

	//Find first parenthesis
	char* parMatch = strchr(response, '(');
	if(parMatch == NULL) {
		return -1;
	}

	//Find last parenthesis
	char* parMatch2 = strchr(response, ')');
	if(parMatch2 == NULL) {
		return -1;
	}

	size_t rawNumLen = parMatch2 - parMatch;

	memcpy(rawConNumbers, parMatch+1, rawNumLen);
	rawConNumbers[rawNumLen] = '\0';
	int n1,n2,n3,n4,n5,n6;
	sscanf(rawConNumbers, "%d,%d,%d,%d,%d,%d", &n1, &n2, &n3, &n4, &n5, &n6);

	*port = (n5 * 256) + n6;
	return 0;
}

int getFileSize(char* response) {
	//Find first parenthesis
	char* parMatch = strchr(response, '(');
	if(parMatch == NULL) {
		return -1;
	}

	//Find last parenthesis
	char* parMatch2 = strchr(response, ')');
	if(parMatch2 == NULL) {
		return -1;
	}

	size_t sizeLen = parMatch2 - parMatch;

	char size[INT_DIGITS + 2 + 7];
	memcpy(size, parMatch+1, sizeLen);
	size[sizeLen] = '\0';
	int sizeN = 0;
	sscanf(size, "%d bytes", &sizeN);
	return sizeN;
}

char* itoa(int i) {
  //Room for INT_DIGITS digits, - and '\0'
  static char buf[INT_DIGITS + 2];

  char *p = buf + INT_DIGITS + 1;	//points to terminating '\0'
  if (i >= 0) {
    do {
      *--p = '0' + (i % 10);
      i /= 10;
    } while (i != 0);
    return p;
  }
  else {			/* i < 0 */
    do {
      *--p = '0' - (i % 10);
      i /= 10;
    } while (i != 0);
    *--p = '-';
  }
  return p;
}