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

int calculatePasvPort(int* port, char* response) {
	char rawConNumbers[MAX_SIZE];

	//Find  first parenthesis
	char* parMatch = strchr(response, '(');
	if(parMatch == NULL) {
		return -1;
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

	*port = numbers[4] * 256 + numbers[5];
	return 0;
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