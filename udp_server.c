/*
 * udpserver.c - A simple UDP echo server
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include "udp.h"
#define MESSAGE "Received exit command!\n"
/*
 * error - wrapper for perror
 */
void error(char *msg)
{
	perror(msg);
	exit(1);
}

/**
 * Exit prints message to client
 */
void recieve_exit_command_handler(Header h, char *buf)
{
	// Send bye bye message
	strcpy(buf, "Bye bye! Hope to see you soon :)\n");
}

/**
 * Ls prints list of files to client
 */
void recieve_ls_command_handler(Header h, char *buf)
{
	strcpy(buf, "Recieved ls command!\n");
	DIR *dir;
    struct dirent *entry;

    // Open the current directory
    dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    // Add each file in the directory to buf
    while ((entry = readdir(dir)) != NULL) {
    	strcat(buf, entry->d_name);
		strcat(buf, "\n");
    }

    // Close dir
    closedir(dir);
}

/**
 * Delete the chosen file
 */
void recieve_delete_command_handler(Header h, char *buf)
{
	strcpy(buf, "Recieved delete command!\n");
}

/**
 * Put the chosen file into server
 */
void recieve_put_command_handler(Header h, char *buf)
{
	strcpy(buf, "Recieved put command!\n");
}

/**
 * Sends the chosen file to client
 */
void recieve_get_command_handler(Header h, char *buf)
{
	strcpy(buf, "Recieved get command!\n");
}

int main(int argc, char **argv)
{
	int sockfd;					   /* socket */
	int portno;					   /* port to listen on */
	int clientlen;				   /* byte size of client's address */
	struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp;		   /* client host info */
	char buf[BUFSIZE];			   /* message buf */
	char *hostaddrp;			   /* dotted decimal host addr string */
	int optval;					   /* flag value for setsockopt */
	int n;						   /* message byte size */

	/*
	 * check command line arguments
	 */
	if (argc != 2)
	{
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	portno = atoi(argv[1]);

	/*
	 * socket: create the parent socket
	 */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	/* setsockopt: Handy debugging trick that lets
	 * us rerun the server immediately after we kill it;
	 * otherwise we have to wait about 20 secs.
	 * Eliminates "ERROR on binding: Address already in use" error.
	 */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
			   (const void *)&optval, sizeof(int));

	/*
	 * build the server's Internet address
	 */
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)portno);

	/*
	 * bind: associate the parent socket with a port
	 */
	if (bind(sockfd, (struct sockaddr *)&serveraddr,
			 sizeof(serveraddr)) < 0)
		error("ERROR on binding");

	/*
	 * main loop: wait for a datagram, then echo it
	 */
	clientlen = sizeof(clientaddr);
	while (1)
	{

		/*
		 * recvfrom: receive a UDP datagram from a client
		 */
		bzero(buf, BUFSIZE);
		n = recvfrom(sockfd, buf, BUFSIZE, 0,
					 (struct sockaddr *)&clientaddr, &clientlen);
		if (n < 0)
			error("ERROR in recvfrom");

		/*
		 * gethostbyaddr: determine who sent the datagram
		 */
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
							  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if (hostp == NULL)
			error("ERROR on gethostbyaddr");
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
			error("ERROR on inet_ntoa\n");
		printf("server received datagram from %s (%s)\n",
			   hostp->h_name, hostaddrp);
		printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);

		/*
		 * handle command
		 */
		Header h;
		memcpy(&h, buf, sizeof(Header));
		
		switch (h.command_id)
		{
		case EXIT:
			recieve_exit_command_handler(h, buf);
			break;
		case LS:
			recieve_ls_command_handler(h, buf);
			break;
		case DELETE:
			recieve_delete_command_handler(h, buf);
			break;
		case PUT:
			recieve_put_command_handler(h, buf);
			break;
		case GET:
			recieve_get_command_handler(h, buf);
			break;

		default:
			break;
		}

		/*
		 * sendto: echo the input back to the client
		 */
		printf("Buf string len: %d\n", strlen(buf));
		n = sendto(sockfd, buf, BUFSIZE-1, 0,
				   (struct sockaddr *)&clientaddr, clientlen);
		if (n < 0)
			error("ERROR in sendto");
	}
}
