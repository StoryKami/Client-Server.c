/*
	Code writer		: 20170517 Lee chanho
	Assignment #	: Assignment 1 for EE323
	File name		: server.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXDATASIZE 1024 // max number of bytes we can get at once
// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr* sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void parse(int argc, char* argv[], int* port, int* host)
{
	if (argc != 5) {
		fprintf(stderr, "ERROR: invalid arguments\n");
		exit(1);
	}
	if (strcmp("-p", argv[1]) == 0 && strcmp("-h", argv[3]) == 0) {
		for (int i = 0; i < strlen(argv[2]); i++) {
			if (isdigit(argv[2][i]) != 0) continue;
			else {
				fprintf(stderr, "ERROR: invalid arguments\n");
				exit(1);
			}
		}
		*port = 2;
		*host = 4;
	}
	else if (strcmp("-h", argv[1]) == 0 && strcmp("-p", argv[3]) == 0) {
		for (int i = 0; i < strlen(argv[4]); i++) {
if (isdigit(argv[4][i]) != 0) continue;
else {
	fprintf(stderr, "ERROR: invalid arguments\n");
	exit(1);
}
		}
		*port = 4;
		*host = 2;
	}
	else {
	fprintf(stderr, "ERROR: invalid arguments\n");
	exit(1);
	}
}

int main(int argc, char* argv[])
{
	int sockfd, numbytes;
	int* PORT;
	int* HOST;
	char buf[MAXDATASIZE];
	struct addrinfo hints, * servinfo, * p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	int ENTER_flag = -1;

	PORT = (int*)malloc(sizeof(int));
	HOST = (int*)malloc(sizeof(int));
	parse(argc, argv, PORT, HOST);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((rv = getaddrinfo(argv[*HOST], argv[*PORT], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr),
		s, sizeof s);
	//printf("client: connecting to %s\n", s);
	freeaddrinfo(servinfo); // all done with this structure
	/*if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
		perror("recv");
		exit(1);
	}
	buf[numbytes] = '\0';
	printf("client: received '%s'\n", buf);*/
	while (1) {

		if (fgets(buf, MAXDATASIZE, stdin) == NULL) {
			if (send(sockfd, "\n\n", 3, 0) == -1)
				perror("send");
			//fprintf(stdout, "end");
			break;
		}

		// Still in the line
		if (buf[strlen(buf) - 1] != '\n') {
			ENTER_flag = 0;
		}
		// first char is newline
		else if (buf[0] == '\n') {
			ENTER_flag++;
			// two ENTERs
			if (ENTER_flag == 2) {	
				if (send(sockfd, "\n\n", 3, 0) == -1)
					perror("send");
				//fprintf(stdout, "end");
				break;
			}
			//very first char is newline
			else if (ENTER_flag == 0) {
				ENTER_flag = 1;
				continue;
			}
		}
		else {	//(buf[strlen(buf) - 1] == '\n') 
			ENTER_flag = 1;
		}
		if (send(sockfd, buf, strlen(buf) + 1, 0) == -1)
			perror("send");
		//fprintf(stdout, "%s", buf);

		// receive a message from the server that "the server finished to handle given message"
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1) 
			perror("send");
	}
	close(sockfd);
	free(PORT);
	free(HOST);
	fprintf(stdout, "\n");
	return 0;
}