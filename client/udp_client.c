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
#include "../udp.h"

int sockfd, portno, n;
int serverlen;
struct sockaddr_in serveraddr;
struct hostent *server;
char *hostname;
char buf[BUFSIZE];

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
	perror(msg);
	exit(0);
}

/**
 * Checks if string is valid file on client
 */
int is_validfile(char* filename) {
	// opening the file in read mode 
    FILE* f = fopen(filename, "r"); 
  
    // checking if the file exist or not 
    if (f == NULL) { 
        printf("File Not Found!\n"); 
        return false; 
    }
	return true;
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
void update_header_in_buffer(Header h) {
	memcpy(buf, &h, sizeof(Header));
}

/**
 * Send buffer to server
 */
void send_to_server(int print) {
	/* send the message to the server */
	serverlen = sizeof(serveraddr);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&serveraddr, serverlen);
	if (n < 0)
		error("ERROR in sendto");

	// TODO ADD TIMER THAT EXITS IF SERVER DOESN'T RESPOND FAST ENOUGH

	/* print the server's reply */
	n = recvfrom(sockfd, buf, BUFSIZE-1, 0, (struct sockaddr *)&serveraddr, &serverlen);
	if (n < 0)
		error("ERROR in recvfrom");
	if (print) {
		printf("Echo from server: %s!!!\n", buf);
	}
}

/**
 * Send exit command
 */
void send_exit_command_handler(Header h) {

	update_header_in_buffer(h);

	send_to_server(1);
}

/**
 * Send ls command
 */
void send_ls_command_handler(Header h) {

	update_header_in_buffer(h);

	send_to_server(1);
}

/**
 * Send delete command
 */
void send_delete_command_handler(Header h) {

	update_header_in_buffer(h);

	send_to_server(1);
	
}

/**
 * Send put command. Will send in separate packages if needed
 */
void send_put_command_handler(Header h) {
	FILE *file;
	size_t bytes_read;

	// Check for valid filename and update size
	if (!is_validfile(h.filename))
		return;

	// Update filesize in header
	h.data_size = get_filesize(h.filename);

	// TODO CALCULATE THE NUMBER OF SENDS REQUIRED
	h.package_number = 1;
	h.total_packages = 1;

	update_header_in_buffer(h);

	// Open file in binary read mode
    file = fopen(h.filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    // Put file data into buf
    bytes_read = fread(buf+sizeof(Header), 1, BUFSIZE-sizeof(Header)-1, file);  // leave space for null terminator
    if (bytes_read == 0 && ferror(file)) {
        perror("Error reading file");
        fclose(file);
        return;
    }

    buf[bytes_read+sizeof(Header)] = '\0';

    // Print the content of the buffer
    printf("File content:\n%s\n", buf+sizeof(Header));

    fclose(file);

	send_to_server(1);
	
}

/**
 * Send get command. Will recieve and write in separate packages if needed
 */
void send_get_command_handler(Header h) {

	// Send request to server
	update_header_in_buffer(h);

	send_to_server(0);

	memcpy(&h, buf, sizeof(Header));

	// Open (new) file
    FILE *file = fopen(h.filename, "wb"); // Open file binary write mode

	// Open file error
	if (file == NULL) {
        perror("Error opening file");
        return;
    }

	printf("%s\n", buf+sizeof(Header));

	// Write to file
    size_t bytes_written = fwrite(buf+sizeof(Header), 1, h.data_size, file);

    // Is fwrite successful, if not, error
    if (bytes_written != h.data_size) {
        perror("Error writing to file");
        fclose(file);
        return;
    }

    // Close file after writing
    fclose(file);
}

/**
 * Handle commands
 */
void send_command_handler(Header h) {
	switch (h.command_id)
	{
	case EXIT:
		send_exit_command_handler(h);
		break;
	case LS:
		send_ls_command_handler(h);
		break;
	case DELETE:
		send_delete_command_handler(h);
		break;
	case PUT:
		send_put_command_handler(h);
		break;
	case GET:
		send_get_command_handler(h);
		break;
	default:
		break;
	}
}

int main(int argc, char **argv)
{
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
	printf("-------- Use the following commands: --------\n"
		   "    get [file_name]\n"
		   "    put [file_name]\n"
		   "    delete [file_name]\n"
		   "    ls\n"
		   "    exit\n"
		   "$$$ ");
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

	/* count the number of words in the command line */
	char *ptrcpy = bufptr;
	int count = 0;
	int inWord = 0;

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

	// fprintf(stdout, "Number of commands %d\n", count); // TODODELETE
	Header h;
	// Check for correct input
	char *token = strtok(bufptr, " ");

	/* EXIT COMMAND HANDLER */
	if (strcmp(token, "exit") == 0)
	{
		h.command_id = EXIT;
		send_command_handler(h);
		return 0;
	}

	/* LS COMMAND HANDLER */
	else if (strcmp(token, "ls") == 0)
	{
		h.command_id = LS;
		send_command_handler(h);
	}

	/* DELETE COMMAND HANDLER */
	else if (strcmp(token, "delete") == 0)
	{
		// Error
		if (count != 2) {
			printf ("Incorrect use of delete.\n");
			continue;
		}

		// Correct command use
		h.command_id = DELETE;
		// Change header
		// printf("Command recognized: %s\n", token); //TODOD
		token = strtok(NULL, " ");
		strcpy(h.filename, token);
		// printf("Command filename: %s\n", h.filename);//TODOD
		send_command_handler(h);
	}

	/* PUT COMMAND HANDLER */
	else if (strcmp(token, "put") == 0)
	{
		// Error
		if (count != 2) {
			printf ("Incorrect use of put.\n");
			continue;
		}

		// Correct command use
		// Change header
		h.command_id = PUT;
		token = strtok(NULL, " ");
		strcpy(h.filename, token);
		send_command_handler(h);
	}

	/* GET COMMAND HANDLER */
	else if (strcmp(token, "get") == 0)
	{
		// Error
		if (count != 2) {
			printf ("Incorrect use of get.\n");
			continue;
		}

		// Correct command use
		// Change header
		h.command_id = GET;
		token = strtok(NULL, " ");
		strcpy(h.filename, token);
		// Get filesize
		send_command_handler(h);
	}

	/* OTHER INPUT HANDLER */
	else
	{
		printf("Unknown command: %s\n", token ? token : "(empty)");
	}

	printf("\n\n");
	}
	return 0;
}
