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

#define CMD_SIZE 1000

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
	while (1) {
		bytes = recv(sockfd, respone, CMD_SIZE, 0);
		if (bytes < 0) {
			fprintf(stderr, "Receiving mess error.\n");
			return;
		}
		if (bytes == 0)
			break;
		respone[bytes] = '\0';
		fprintf(stdout, "%s", respone);
	}
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
	if (sockfd != -1)
		grab_some_popcorn(sockfd, argv[1], argv[2]);
	close(sockfd);
	return 0;
}
