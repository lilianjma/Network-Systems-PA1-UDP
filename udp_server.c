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

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
	perror(msg);
	exit(1);
}

/*
* sendto: echo the input back to the client
*/
void send_to_client(char* buf, int sockfd, struct sockaddr_in clientaddr, int clientlen) {

	// printf("Buf string len: %lu\n", strlen(buf));
	// printf("%s\n", buf);
	int n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, clientlen);
	// bzero(buf, BUFSIZE);
	printf("Middle senttoclient;");

	if (n < 0)
		error("ERROR in sendto");
	printf("End senttoclient;");
}

void recieve_from_client(char* rbuf, int sockfd, struct sockaddr_in clientaddr, int clientlen) {
	bzero(rbuf, BUFSIZE);
	int n = recvfrom(sockfd, rbuf, BUFSIZE, 0,
					(struct sockaddr *)&clientaddr, &clientlen);
	if (n < 0)
		error("ERROR in recvfrom");
}

/**
 * Checks if string is valid file on client
 */
int is_validfile(char* filename, char* buf, Header h) {
	// opening the file in read mode 
    FILE* f = fopen(filename, "r"); 
  
    // checking if the file exist or not 
    if (f == NULL) { 
		strcpy(h.filename, "-1\n");
        memcpy(buf, &h, sizeof(Header)); 
        return FALSE; 
    }

	return TRUE;
}

/**
 * Return file size of input file name
 */
long int get_filesize(char* filename) 
{ 
    // opening the file in read mode 
    FILE* f = fopen(filename, "r"); 
  
    // checking if the file exist or not 
    if (f == NULL) { 
        printf("File Not Found!\n"); 
        return -1; 
    }
  
    fseek(f, 0L, SEEK_END);
    long int res = ftell(f);
    fclose(f);

    return res; 
}

/**
 * Copy the header into the first bytes of the buffer
 */
void update_header_in_buffer(Header h, char* buf) {
	memcpy(buf, &h, sizeof(Header));
}

/**
 * Exit prints message to client
 */
void recieve_exit_command_handler(Header h, char* rbuf, char* buf, int sockfd, struct sockaddr_in clientaddr, int clientlen)
{
	// Send bye bye message
	
	strcpy(buf, "Bye bye! Hope to see you soon :)\n");
	
	send_to_client(buf, sockfd, clientaddr, clientlen);
}

/**
 * Ls prints list of files to client
 */
void recieve_ls_command_handler(Header h, char* rbuf, char* buf, int sockfd, struct sockaddr_in clientaddr, int clientlen)
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

	// Send to client
	send_to_client(buf, sockfd, clientaddr, clientlen);
}

/**
 * Delete the chosen file
 */
void recieve_delete_command_handler(Header h, char* rbuf, char* buf, int sockfd, struct sockaddr_in clientaddr, int clientlen)
{
	
	strcpy(buf, "Recieved delete command!\n");

	// Check if file exists
	if (!is_validfile(h.filename, buf, h)){
		send_to_client(buf, sockfd, clientaddr, clientlen);
		return;
	}

	// Start delete file
	const char *filename = h.filename;

	// Delete file
    if (remove(filename) == 0) {
        strcat(buf, "Deleted ");
		strcat(buf, filename);
    } else {
        perror("Error deleting file");
    }

	strcat(buf, "\n");

	// Send to client
	send_to_client(buf, sockfd, clientaddr, clientlen);
}

/**
 * Put the chosen file into server
 */
void recieve_put_command_handler(Header h, char* rbuf, char* buf, int sockfd, struct sockaddr_in clientaddr, int clientlen)
{
	size_t datasize;

	// Add message
	strcpy(buf, "Recieved put command!\n");

	// Open file binary write mode 
	printf("filename: %s\n", h.filename);
    FILE *file = fopen(h.filename, "wb"); 

	// Open file error
	if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

	printf("%s\n", rbuf+sizeof(Header));

	while (1) {
		if ( (DATABUFSIZE * (h.package_number + 1)) < h.data_size) {
			datasize = DATABUFSIZE;
		} else {
			datasize = h.data_size%DATABUFSIZE;
		}

		printf("%s", rbuf+sizeof(Header));
		// Write to file
		size_t bytes_written = fwrite(rbuf+sizeof(Header), 1, datasize, file);

		// If fwrite not successful error
		if (bytes_written != datasize) {
			perror("Error writing to file");
			fclose(file);
			exit(1);
		}

		fseek(file, datasize * (h.package_number + 1), SEEK_SET);

		// Send to client
		printf("HELLOHELP\n");
		strcpy(buf, "hello help!\n");
		send_to_client(buf, sockfd, clientaddr, clientlen);

		if (h.package_number == h.total_packages) {
			break;
		}
		// Recieve from client
		int n = recvfrom(sockfd, rbuf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
		if (n < 0)
			error("ERROR in recvfrom");

		// Update header
		memcpy(&h, rbuf, sizeof(Header));
	}
	printf("Close File");
    // Close file after writing
    fclose(file);
}

/**
 * Sends the chosen file to client
 */
void recieve_get_command_handler(Header h, char* rbuf, char* buf, int sockfd, struct sockaddr_in clientaddr, int clientlen)
{
	FILE *file;
	size_t bytes_read;

	// Check for valid filename and update size
	if (!is_validfile(h.filename, buf, h)){
		send_to_client(buf, sockfd, clientaddr, &clientlen);
		return;
	}
	
	// Update filesize in header
	h.data_size = get_filesize(h.filename);

	// Calculate number of packages required
	h.package_number = 0;
	h.total_packages = h.data_size / DATABUFSIZE;
	update_header_in_buffer(h, buf);

	// Open file in binary read mode
    file = fopen(h.filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

	// Read file until done
	while (h.package_number <= h.total_packages) {
		// Put file data into buf
		bytes_read = fread(buf+sizeof(Header), 1, DATABUFSIZE, file);  // leave space for null terminator ?
		if (bytes_read == 0 && ferror(file)) {
			perror("Error reading file");
			fclose(file);
			return;
		}

		printf("Sending to client");
		send_to_client(buf, sockfd, clientaddr, &clientlen);

		// Recieve from client
		int n = recvfrom(sockfd, rbuf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
		if (n < 0)
			error("ERROR in recvfrom");

		// Update header numbers
		h.package_number++;
		update_header_in_buffer(h, buf);
	}

	fclose(file);
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
	char rbuf[BUFSIZE];			   /* recieve buf */
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
		bzero(rbuf, BUFSIZE);
		n = recvfrom(sockfd, rbuf, BUFSIZE, 0,
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
		printf("server received %lud/%d bytes: %s\n", strlen(rbuf), n, rbuf);

		/*
		 * handle command
		 */
		Header h;
		memcpy(&h, rbuf, sizeof(Header));
		
		switch (h.command_id)
		{
		case EXIT:
			printf("EXIT\n");
			recieve_exit_command_handler(h, rbuf, buf, sockfd, clientaddr, clientlen);
			break;
		case LS:
			printf("LS\n");
			recieve_ls_command_handler(h, rbuf, buf, sockfd, clientaddr, clientlen);
			break;
		case DELETE:
			printf("DELETE\n");
			recieve_delete_command_handler(h, rbuf, buf, sockfd, clientaddr, clientlen);
			break;
		case PUT:
			printf("PUT\n");
			recieve_put_command_handler(h, rbuf, buf, sockfd, clientaddr, clientlen);
			break;
		case GET:
			printf("GET\n");
			recieve_get_command_handler(h, rbuf, buf, sockfd, clientaddr, clientlen);
			break;
		default:
			printf("DEF\n");
			break;
		}

		memset(&h, 0, sizeof(Header));
	}
}
