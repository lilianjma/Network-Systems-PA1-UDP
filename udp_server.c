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

	printf("Buf string len: %d\n", strlen(buf));
	int n = sendto(sockfd, buf, BUFSIZE-1, 0,
				(struct sockaddr *)&clientaddr, clientlen);
	if (n < 0)
		error("ERROR in sendto");
}

/**
 * Checks if string is valid file on client
 */
int is_validfile(char* filename, char* buf) {
	// opening the file in read mode 
    FILE* f = fopen(filename, "r"); 
  
    // checking if the file exist or not 
    if (f == NULL) { 
        strcpy(buf, "File Not Found!\n"); 
        return 0; 
    }
	return 1;
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
	if (!is_validfile(h.filename, buf)){
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
	strcpy(buf, "Recieved put command!\n");

    FILE *file = fopen(h.filename, "wb"); // Open file binary write mode

	// Open file error
	if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

	printf("%s\n", rbuf+sizeof(Header));

	// Write to file
    size_t bytes_written = fwrite(rbuf+sizeof(Header), 1, h.data_size, file);

    // Is fwrite successful, if not, error
    if (bytes_written != h.data_size) {
        perror("Error writing to file");
        fclose(file);
        exit(1);
    }

    // Close file after writing
    fclose(file);

	// Send to client
	send_to_client(buf, sockfd, clientaddr, clientlen);}

/**
 * Sends the chosen file to client
 */
void recieve_get_command_handler(Header h, char* rbuf, char* buf, int sockfd, struct sockaddr_in clientaddr, int clientlen)
{
	// Check if file exists
	if (!is_validfile(h.filename, buf)){
		send_to_client(buf, sockfd, clientaddr, clientlen);
		return;
	}

	// Start sending file
	FILE *file;
	size_t bytes_read;

	// Update filesize in header
	h.data_size = get_filesize(h.filename);

	// TODO CALCULATE THE NUMBER OF SENDS REQUIRED
	h.package_number = 1;
	h.total_packages = 1;

	// Update buffer with header
	update_header_in_buffer(h, buf);

	// Open file in binary read mode
    file = fopen(h.filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    // Put file data into buf
    bytes_read = fread(buf+sizeof(Header), 1, h.data_size, file);  // leave space for null terminator
    if (bytes_read == 0 && ferror(file)) {
        perror("Error reading file");
        fclose(file);
        return;
    }

	// Add null terminator to end of file
    buf[bytes_read+sizeof(Header)] = '\0';

    // Print the content of the buffer
    printf("File content:\n%s\n", buf+sizeof(Header));

    fclose(file);

	// Send to client
	send_to_client(buf, sockfd, clientaddr, clientlen);
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
		printf("server received %d/%d bytes: %s\n", strlen(rbuf), n, rbuf);

		/*
		 * handle command
		 */
		Header h;
		memcpy(&h, rbuf, sizeof(Header));
		
		switch (h.command_id)
		{
		case EXIT:
			recieve_exit_command_handler(h, rbuf, buf, sockfd, clientaddr, clientlen);
			break;
		case LS:
			recieve_ls_command_handler(h, rbuf, buf, sockfd, clientaddr, clientlen);
			break;
		case DELETE:
			recieve_delete_command_handler(h, rbuf, buf, sockfd, clientaddr, clientlen);
			break;
		case PUT:
			recieve_put_command_handler(h, rbuf, buf, sockfd, clientaddr, clientlen);
			break;
		case GET:
			recieve_get_command_handler(h, rbuf, buf, sockfd, clientaddr, clientlen);
			break;

		default:
			break;
		}
	}
}
