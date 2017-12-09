#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include "parsingLink.h"

#define CMD_SIZE 1000
#define HTML_FILENAME "respone"
#define ACTUAL_FILE "file"

void splitMessageAndFile(char* fileName) {
	FILE* fo = fopen(fileName, "rb");
	char *header, *content;
	int lenHeader, lenContent;
	char c;
	int whichHTTP;	// 0: 1.0  	or  1: 1.1
	int chunked;	// 0: no	or 	1: yes
	
	whichHTTP = -1;
	chunked = 0;

	// Read message
	for (header = NULL, lenHeader = 0; fread(&c, sizeof(char), 1, fo) > 0; ) {
		lenHeader = insertChar(c, &header, lenHeader);
		if (whichHTTP == -1 && lenHeader >= 8 && strcmp(header+lenHeader-8, "HTTP/1.1") == 0) {
			whichHTTP = 1;
		}
		else if (whichHTTP == -1 && lenHeader >= 4 && strcmp(header+lenHeader-8, "HTTP/1.0") == 0) {
			whichHTTP = 0;
		}
		else if (!chunked && lenHeader >= 26 && strcmp(header+lenHeader-26, "Transfer-Encoding: chunked") == 0) {
			chunked = 1;
		}
		else if (lenHeader >= 4 && strcmp(header+lenHeader-4, "\r\n\r\n") == 0) {
			break;
		}
	}
	
	puts(header);

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
	
	fo = fopen("file", "wb");
	fwrite(content, sizeof(char), lenContent, fo);
	fclose(fo);
}

void grab_some_popcorn(int sockfd, char *host, char *path) {
	char *cmd_fmt = "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
	char *cmd = malloc(CMD_SIZE * sizeof(char));
	int cmd_len = sprintf(cmd, cmd_fmt, path, host);
	int sent = 0, bytes;
	do {
		bytes = send(sockfd, cmd + sent, cmd_len - sent, 0);
		if (bytes < 0) {
			fprintf(stderr, "Sending mess error.\n");
			return;
		}
		if (bytes == 0)
			break;
		sent += bytes;
	} while (sent < cmd_len);

	char *respone = malloc((CMD_SIZE + 1) * sizeof(char));
	
	FILE* fo = fopen(HTML_FILENAME, "wb");
	while (1) {
		bytes = recv(sockfd, respone, CMD_SIZE, 0);
		if (bytes < 0) {
			fprintf(stderr, "Receiving mess error.\n");
			return;
		}
		if (bytes == 0)
			break;
		respone[bytes] = '\0';

		int i;
		fwrite(respone, sizeof(char), bytes, fo);
	}
	fclose(fo);
}

int main(int argc, char *argv[])
{
	struct addrinfo hints, *res, *p;
	int status, sockfd;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(argv[1], "http", &hints, &res)) != 0) {
		fprintf(stderr, "Cannot find address of %s\n", argv[1]);
		return 1;
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
	if (sockfd != -1) {
		grab_some_popcorn(sockfd, argv[1], argv[2]);
/*	
		EntryList L;
		L = parsingFile(HTML_FILENAME);
		
		Entry* e;
		for (e = L.head; e; e = e->next_entry)
			printf("url=%s\nname=%s\n", e->url, e->name);
*/
	}
	close(sockfd);
	return 0;
}
