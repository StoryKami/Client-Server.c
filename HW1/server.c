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
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MAXDATASIZE 1024 // max number of bytes we can get at once
#define BACKLOG 10

void sigchld_handler(int s)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr* sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

 // parse the command
void parse(int argc, char *argv[]) 
{
	if (argc != 3) {
		fprintf(stderr, "ERROR: invalid arguments\n");
		exit(1);
	}
	if (strcmp("-p", argv[1]) == 0) {

		for (int i = 0; i < strlen(argv[2]); i++) {
			if (isdigit(argv[2][i]) != 0) continue;
			else {
				fprintf(stderr, "ERROR: invalid arguments\n");
				exit(1);
			}
		}
	}
	else {
		fprintf(stderr, "ERROR: invalid arguments\n");
		exit(1);
	}
}

int main(int argc, char* argv[])
{
	int sockfd, new_fd, numbytes; // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, * servinfo, * p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	char buf[MAXDATASIZE];
	int ENTER_flag = 0;

	parse(argc, argv);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[2], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
			sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}
	freeaddrinfo(servinfo); // all done with this structure
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
	//fprintf(stdout, "server: waiting for connections...\n");
	while (1) { // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr*) & their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr*) & their_addr),
			s, sizeof s);
	//	fprintf(stdout, "server: got connection from %s\n", s);
		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener

			while ((numbytes = recv(new_fd, buf, MAXDATASIZE, 0)) != -1) {
				// connection end
				if (strcmp(buf, "\n\n") == 0) {
					if (ENTER_flag ==1) fprintf(stdout, "\n");	//Last message has no newline char
					break;
				}

				if (buf[strlen(buf) - 1] != '\n')	// message has no newline char
					ENTER_flag = 1;
				else ENTER_flag = 0;

				fprintf(stdout, "%s", buf);	//display the message

				if (send(new_fd, "fin", 4, 0) == -1)	// reply to the client that "display finished so you can send more messages"
					perror("send");
			}
		//	fprintf(stdout, "client: connection end\n");
			close(new_fd);
			exit(0);
		}
		close(new_fd); // parent doesn't need this
	}
	return 0;
}