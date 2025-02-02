/*
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFSIZE 1024
#define EXIT 0
#define LS 1
#define DELETE 2
#define PUT 3
#define GET 4

/**
 * package header
 */
typedef struct
{
	uint8_t command_id;
	uint32_t num_bytes;
} Header;

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
	perror(msg);
	exit(0);
}

int main(int argc, char **argv)
{
	int sockfd, portno, n;
	int serverlen;
	struct sockaddr_in serveraddr;
	struct hostent *server;
	char *hostname;
	char buf[BUFSIZE];

	/* check command line arguments */
	if (argc != 3)
	{
		fprintf(stderr, "usage: %s <hostname> <port>\n", argv[0]);
		exit(0);
	}
	hostname = argv[1];
	portno = atoi(argv[2]);

	/* socket: create the socket */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	/* gethostbyname: get the server's DNS entry */
	server = gethostbyname(hostname);
	if (server == NULL)
	{
		fprintf(stderr, "ERROR, no such host as %s\n", hostname);
		exit(0);
	}

	/* build the server's Internet address */
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr,
		  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
	serveraddr.sin_port = htons(portno);

	while (1)
	{
	/* get a message from the user */
	bzero(buf, BUFSIZE);
	printf("Use the following commands:\n"
		   "    get [file_name]\n"
		   "    put [file_name]\n"
		   "    delete [file_name]\n"
		   "    ls\n"
		   "    exit\n\n");
	fgets(buf, BUFSIZE, stdin);

	/* trim leading spaces and new line */
	buf[strcspn(buf, "\n")] = '\0';

	char *bufptr = buf;
	while (*bufptr == ' ')
	{
		bufptr++;
	}

	/* trim trailing spaces */
	char *endptr = bufptr + strlen(bufptr) - 1;
	while (endptr > bufptr && *endptr == ' ') // move pointer backwords as long as is before beginning and equal to ' '
	{
		endptr--;
	}

	*(endptr + 1) = '\0';

	/* count the number of words in the command line*/
	char *ptrcpy = bufptr;
	int count = 0;
	int inWord = 0;

	printf("Innit\n");
	while (*ptrcpy != '\0')
	{
		if (*ptrcpy == ' ')
		{
			inWord = 0;
		}
		else if (!inWord)
		{
			inWord = 1;
			count++;
		}
		ptrcpy++;
	}

	fprintf(stdout, "Number of commands %d\n", count);

	// Create header
	Header h;

	// Check for correct input
	char *token = strtok(bufptr, " ");

	if (strcmp(token, "exit") == 0)
	{
		h.command_id = 0;
		printf("Command recognized: %s\n", token);
	}
	else if (strcmp(token, "ls") == 0)
	{
		h.command_id = 1;
		printf("Command recognized: %s\n", token);
	}
	else if (strcmp(token, "delete") == 0)
	{
		h.command_id = 2;
		printf("Command recognized: %s\n", token);
	}
	else if (strcmp(token, "put") == 0)
	{
		h.command_id = 3;
		printf("Command recognized: %s\n", token);
	}
	else if (strcmp(token, "get") == 0)
	{
		h.command_id = 4;
		printf("Command recognized: %s\n", token);
	}
	else
	{
		h.command_id = 0xFF;
		printf("Unknown command: %s\n", token ? token : "(empty)");
	}
	token = strtok(NULL, bufptr);

	/* send the message to the server */
	serverlen = sizeof(serveraddr);
	n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
	if (n < 0)
		error("ERROR in sendto");

	/* print the server's reply */
	n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
	if (n < 0)
		error("ERROR in recvfrom");
	printf("Echo from server: %s!!!\n", buf);
	}
	return 0;
}
