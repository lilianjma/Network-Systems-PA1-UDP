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
#define FILENAME_BUFSIZE 1024
#define EXIT 0
#define LS 1
#define DELETE 2
#define PUT 3
#define GET 4
#define false 0
#define true 1

/**
 * package header
 */
typedef struct
{
	uint8_t command_id;
	char filename[FILENAME_BUFSIZE];
	long int num_bytes;
} Header;

Header h;

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

void command_handler() {
	switch (h.command_id)
	{
	case EXIT:
		// exit_command_handler();
		break;
	case LS:
		// ls_command_handler();
		break;
	case DELETE:
		if (!is_validfile(h.filename))
			return;
		printf("Filesize: %ld\n", get_filesize(h.filename));
		// delete_command_handler();
		break;
	case PUT:
		if (!is_validfile(h.filename))
			return;
		// put_command_handler();
		break;
	case GET:
		if (!is_validfile(h.filename))
			return;
		// get_command_handler();
		break;
	
	default:
		break;
	}
}
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
	printf("--- Use the following commands: ---\n"
		   "    get [file_name]\n"
		   "    put [file_name]\n"
		   "    delete [file_name]\n"
		   "    ls\n"
		   "    exit\n\n$$$ ");
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

	// fprintf(stdout, "Number of commands %d\n", count); // TODODELETE

	// Check for correct input
	char *token = strtok(bufptr, " ");

	/* EXIT COMMAND HANDLER */
	if (strcmp(token, "exit") == 0)
	{
		h.command_id = EXIT;
		printf("Command recognized: %s\n", token);
		command_handler();
		return 0;
	}

	/* LS COMMAND HANDLER */
	else if (strcmp(token, "ls") == 0)
	{
		h.command_id = LS;
		printf("Command recognized: %s\n", token);
		command_handler();
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
		command_handler();
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
		command_handler();
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
		command_handler();
	}

	/* OTHER INPUT HANDLER */
	else
	{
		h.command_id = 0xFF;
		printf("Unknown command: %s\n", token ? token : "(empty)");
	}

	printf("\n\n");

	// /* send the message to the server */
	// serverlen = sizeof(serveraddr);
	// n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
	// if (n < 0)
	// 	error("ERROR in sendto");

	// /* print the server's reply */
	// n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
	// if (n < 0)
	// 	error("ERROR in recvfrom");
	// printf("Echo from server: %s!!!\n", buf);
	}
	return 0;
}
