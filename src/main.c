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

char respone[BLOCK_SIZE];

int readSignal(FILE* fo, char** header, int* lenHeader) {
	char c;
	int res;
	res = 0;
	
	while (fread(&c, sizeof(char), 1, fo) > 0) {
		*lenHeader = insertChar(c, header, *lenHeader);
		if (c >= '0' && c <= '9') {
			res = (res*10)+(c-'0');
		}
		else if (strcmp((*header)+(*lenHeader)-2, "\r\n") == 0) {
			break;
		}
	}
	return res;
}

int readHeader(FILE* fo, int* chunked, int* whichHTTP) {
	char* header;
	int lenHeader;
	char c;
	int res;
	
	*chunked = 0;
	*whichHTTP = -1;	
	res = 0;

	for (header = NULL, lenHeader = 0; fread(&c, sizeof(char), 1, fo) > 0; ) {
		lenHeader = insertChar(c, &header, lenHeader);
		if (*whichHTTP == -1 && lenHeader >= 8 && strcmp(header+lenHeader-8, "HTTP/1.1") == 0) {
			*whichHTTP = 1;
			res = readSignal(fo, &header, &lenHeader);
		}
		else if (*whichHTTP == -1 && lenHeader >= 4 && strcmp(header+lenHeader-8, "HTTP/1.0") == 0) {
			*whichHTTP = 0;
			res = readSignal(fo, &header, &lenHeader);
		}
		else if (!(*chunked) && lenHeader >= 26 && strcmp(header+lenHeader-26, "Transfer-Encoding: chunked") == 0) {
			*chunked = 1;
		}
		else if (lenHeader >= 4 && strcmp(header+lenHeader-4, "\r\n\r\n") == 0) {
			break;
		}
	}
	free(header);
	return res;
}

int readContent(FILE* fo, FILE* fr, int chunked) {
	char* content;
	int lenContent;
	char c;

	if (!chunked) {
		content = (char*)malloc(BLOCK_SIZE * sizeof(char));
		while ((lenContent = fread(content, sizeof(char), BLOCK_SIZE, fo)) > 0) {
			fwrite(content, sizeof(char), lenContent, fr);
		}
	}
	else {
		for (content = NULL, lenContent = 0; ; ) {
			lenContent = 0;

			int sz;
			sz = 0;
			while (fread(&c, sizeof(char), 1, fo) > 0) {
				lenContent = insertChar(c, &content, lenContent);
				if (lenContent >= 2 && strcmp(content+lenContent-2, "\r\n") == 0) {
					lenContent -= 2;
					break;
				}
			}
			if (lenContent == 0)
				break;

			sz = strtol(content, NULL, 16);
			
			content = (char*)realloc(content, sz * sizeof(char));
			sz = fread(content, sizeof(char), sz, fo);
			fwrite(content, sizeof(char), sz, fr);
			
			fread(&c, sizeof(char), 1, fo);	// read '\r'
			fread(&c, sizeof(char), 1, fo); // read '\n'
		}
	}
	free(content);
}

int splitMessageAndFile(char* fileName, char* fileResult) {
	FILE* fo = fopen(fileName, "rb");
	FILE* fr = fopen(fileResult, "wb");

	int whichHTTP;	// 0: 1.0  	or  1: 1.1
	int chunked;	// 0: no	or 	1: yes
	int res;
	
	res = readHeader(fo, &chunked, &whichHTTP);
	readContent(fo, fr, chunked);
	fclose(fo);
	fclose(fr);
	return res;
}

int newSocketAndConnect(char* host, char* path) {
	struct addrinfo hints, *res, *p;
	int status, sockfd;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(host, "http", &hints, &res)) != 0) {
		fprintf(stderr, "Cannot find address of %s\n", host);
		return -1;
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
	return sockfd;
}

void sendMess(char* host, char* path, int sockfd, char* cmd, int cmd_len) {
	int sent, bytes;

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

}

void receiveMess(char* host, char* path, int sockfd) {
	int bytes;
	FILE* fmess;
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

}

void grab_some_popcorn(char *host, char *path) {
	int sockfd;
	FILE* fmess;
	char *cmd;
	int cmd_len;
	
	sockfd = newSocketAndConnect(host, path);

	cmd = malloc((FMT_CMD_LEN + strlen(host) + strlen(path) + 1) * sizeof(char));	// plus 1 for '\0' character
	if (path[0] == '/')
		cmd_len = sprintf(cmd, FMT_CMD, path+1, host);
	else
		cmd_len = sprintf(cmd, FMT_CMD, path, host);
	sendMess(host, path, sockfd, cmd, cmd_len);
	free(cmd);
	
	receiveMess(host, path, sockfd);
	close(sockfd);
}

void downloading(char* host, char* path, char* curpath, char* fname) {	// path is either file or folder
	int res, p;
	char *localpath, *subpath;
	localpath = (char*)malloc((strlen(curpath)+1+strlen(path)+FN_MAX+1) * sizeof(char));
	subpath = (char*)malloc((strlen(path)+1+FN_MAX+1) * sizeof(char));

	if (fname[0] != '\0' && stat(curpath, &st) == -1) {	// make new directory if not exists
		mkdir(curpath, 0700);
	}

	if (fname[0] == '\0') {
		p = strlen(path);
		if (p > 0) {
			if (path[p-1] == '/') --p;
			while (p > 0 && path[p-1] != '/')
				--p;
		}
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
		grab_some_popcorn(host, subpath);
		res = splitMessageAndFile(MESSAGE_FILENAME, TEMP_FILENAME);
	}

	if (res != 200) {	// not a directory, i.e. a file
		if (fname[0] == '\0') {	// Null string
			sprintf(localpath, "%s%s", curpath, path+p);
		}
		else {
			sprintf(localpath, "%s/%s", curpath, fname);
		}
		printf("Downloading %s\n", path);
		grab_some_popcorn(host, path);
		splitMessageAndFile(MESSAGE_FILENAME, localpath);
	}
	else {
		if (fname[0] == '\0') {
			sprintf(localpath, "%s%s", curpath, path+p);
			mkdir(localpath, 0700);
		}
		EntryList L;
		L = parsingFile(TEMP_FILENAME);
		
		Entry* e;
		for (e = L.head; e; e = e->next_entry) {
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
				e->url[j--] = '\0';
			
			if (((j >= 2 && e->url[j-2] == '/') || j == 1) && e->url[j] == '.' && e->url[j-1] == '.')
				continue;

			for ( ; j >= i; --j) {
				if (e->url[j] == '\"' || e->url[j] == '*' || e->url[j] == '/' 
					|| e->url[j] == ':' || e->url[j] == '<' || e->url[j] == '>'
					|| e->url[j] == '?' || e->url[j] == '\\' || e->url[j] == '|') {
					flag = 1;
					break;
				}
			}
			
			if (flag != 0) continue;
			
			if (fname[0] == '\0')
				sprintf(localpath, "%s%s", curpath, path+p);
			else
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
	int i, u, v, len;
	for (i = 0, len = strlen(argv[1]), u = -1, v = len; i < len; ++i) {
		if (argv[1][i] == '/' && argv[1][i+1] != '/') {
			if (u == -1) {
				u = i+1;
			}
			else if (u >= 0) {
				v = i+1;
				argv[1][i] = '\0';
				break;
			}
		}
	}
	if (u == -1) 
		return 0;
	
	downloading(argv[1]+u, argv[1]+v, "1512203_1512525_", "");
	return 0;
}
