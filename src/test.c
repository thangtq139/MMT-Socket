#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include "parsingLink.h"

#define BLOCK_SIZE 1024
#define MESSAGE_FILENAME "MESS"
#define TEMP_FILENAME "TEMP"
#define BUFF_FILENAME "BUFFER"
#define FN_MAX 255

char* FMT_CMD = "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
int FMT_CMD_LEN = 45;
struct stat st = {0};

int splitMessageAndFile(char* fileName, char* fileResult) {
	FILE* fo = fopen(fileName, "rb");
	char *header, *content;
	int lenHeader, lenContent;
	char c;
	int whichHTTP;	// 0: 1.0  	or  1: 1.1
	int chunked;	// 0: no	or 	1: yes
	int res;
	
	whichHTTP = -1;
	chunked = 0;
	res = 0;

	// Read message
	for (header = NULL, lenHeader = 0; fread(&c, sizeof(char), 1, fo) > 0; ) {
		lenHeader = insertChar(c, &header, lenHeader);
		if (whichHTTP == -1 && lenHeader >= 8 && strcmp(header+lenHeader-8, "HTTP/1.1") == 0) {
			whichHTTP = 1;
			
			while (fread(&c, sizeof(char), 1, fo) > 0) {
				lenHeader = insertChar(c, &header, lenHeader);
				if (c >= '0' && c <= '9') {
					res = (res*10)+(c-'0');
				}
				else if (strcmp(header+lenHeader-2, "\r\n") == 0) {
					break;
				}
			}
		}
		else if (whichHTTP == -1 && lenHeader >= 4 && strcmp(header+lenHeader-8, "HTTP/1.0") == 0) {
			whichHTTP = 0;
			
			while (fread(&c, sizeof(char), 1, fo) > 0) {
				lenHeader = insertChar(c, &header, lenHeader);
				if (c >= '0' && c <= '9') {
					res = (res*10)+(c-'0');
				}
				else if (strcmp(header+lenHeader-2, "\r\n") == 0) {
					break;
				}
			}
		}
		else if (!chunked && lenHeader >= 26 && strcmp(header+lenHeader-26, "Transfer-Encoding: chunked") == 0) {
			chunked = 1;
		}
		else if (lenHeader >= 4 && strcmp(header+lenHeader-4, "\r\n\r\n") == 0) {
			break;
		}
	}
	
	int lastpos;
	lastpos = 0;

	for (content = NULL, lenContent = 0; fread(&c, sizeof(char), 1, fo) > 0; ) {
		lenContent = insertChar(c, &content, lenContent);
		
		if (chunked && lenContent >= 2 && strcmp(content+lenContent-2, "\r\n") == 0) {
			if (lastpos == -1) {
				lastpos = lenContent-2;
			}
			else {
				lenContent = lastpos;
				lastpos = -1;
			}
		}
			
	}
	fclose(fo);

	if (chunked && lastpos != -1) {
		lenContent = lastpos;
	}
	
	fo = fopen(fileResult, "wb");
	fwrite(content, sizeof(char), lenContent, fo);
	fclose(fo);
	return res;

	free(header);
	free(content);
}

void grab_some_popcorn(char *host, char *path) {
	struct addrinfo hints, *res, *p;
	int status, sockfd;
	FILE* fmess;
	char *cmd, *respone;
	int cmd_len;
	int sent, bytes;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(host, "http", &hints, &res)) != 0) {
		fprintf(stderr, "Cannot find address of %s\n", host);
		return ;
	}
	sockfd = -1;
	for (p = res; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd < 0) {
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
			continue;
		}
		break;
	}

	cmd = malloc((FMT_CMD_LEN + strlen(host) + strlen(path) + 1) * sizeof(char));	// plus 1 for '\0' character
	if (path[0] == '/')
		cmd_len = sprintf(cmd, FMT_CMD, path+1, host);
	else
		cmd_len = sprintf(cmd, FMT_CMD, path, host);
	
	sent = 0;
	do {
		bytes = send(sockfd, cmd + sent, cmd_len - sent, 0);
		if (bytes < 0) {
			fprintf(stderr, "Sending mess error.\n");
			return;
		}
		else if (bytes == 0)
			break;
		sent += bytes;
	} while (sent < cmd_len);
	free(cmd);
	
	respone = malloc(BLOCK_SIZE * sizeof(char));
	fmess = fopen(MESSAGE_FILENAME, "wb");
	while (1) {
		bytes = recv(sockfd, respone, BLOCK_SIZE, 0);
		if (bytes < 0) {
			fprintf(stderr, "Receiving mess error.\n");
			return;
		}
		else if (bytes == 0) {
			break;
		}
		fwrite(respone, sizeof(char), bytes, fmess);
	}
	fclose(fmess);
	free(respone);

	close(sockfd);
}


void downloading(char* host, char* path, char* curpath, char* fname) {	// path is either file or folder
	int res;
	char *localpath, *subpath;
	localpath = (char*)malloc((strlen(curpath)+1+strlen(fname)+1) * sizeof(char));
	subpath = (char*)malloc((strlen(path)+1+FN_MAX+1) * sizeof(char));

	if (stat(curpath, &st) == -1) {	// make new directory if not exists
		mkdir(curpath, 0700);
	}

	if (path[strlen(path)-1] == '/') {
		grab_some_popcorn(host, path);
		res = splitMessageAndFile(MESSAGE_FILENAME, TEMP_FILENAME);
		if (res != 200) {
			free(localpath);
			free(subpath);
			return;
		}
	}
	else {
		sprintf(subpath, "%s/", path);	// '/' character is for directory
		printf("Downloading %s\n", path);
		grab_some_popcorn(host, subpath);
		res = splitMessageAndFile(MESSAGE_FILENAME, TEMP_FILENAME);
	}

	if (res != 200) {	// not a directory
		sprintf(localpath, "%s/%s", curpath, fname);
		
		grab_some_popcorn(host, path);
		splitMessageAndFile(MESSAGE_FILENAME, localpath);
	}
	else {
		EntryList L;
		L = parsingFile(TEMP_FILENAME);
		
		Entry* e;
		for (e = L.head; e; e = e->next_entry) {
			puts(e->url);
			int flag;
			int i, j;

			
			if (e->url[i] == '/') {
				for (i = 0; path[i] && e->url[i]; ++i) 
					if (path[i] != e->url[i]) 
						break;

				if (path[i] != '\0' || e->url[i] != '/') 
					continue;
				
				++i;
				if (e->url[i+1] == '\0')
					continue;
			}
			else {
				i = 0;
			}

			flag = 0;

			j = strlen(e->url)-1;
			if (e->url[j] == '/')
				--j;

			for ( ; j >= i; --j) {
				if (e->url[j] == '\"' || e->url[j] == '*' || e->url[j] == '/' 
					|| e->url[j] == ':' || e->url[j] == '<' || e->url[j] == '>'
					|| e->url[j] == '?' || e->url[j] == '\\' || e->url[j] == '|'
					|| e->url[j] == '-') {
					flag = 1;
					break;
				}
			}
			if (flag != 0) continue;

//			printf("%s\n%s\n", path, e->url);
			
			sprintf(localpath, "%s/%s", curpath, fname);
			if (path[strlen(path)-1] != '/')
				sprintf(subpath, "%s/%s", path, e->url+i);
			else
				sprintf(subpath, "%s%s", path, e->url+i);
			downloading(host, subpath, localpath, e->name);
		}
		clearList(&L);
	}
	
	free(localpath);
	free(subpath);
}


int main(int argc, char *argv[]) {
	downloading(argv[1], argv[2], argv[1], "");
	return 0;
}
