#include <stdio.h>
#include <stdlib.h>

#define BUFSIZE 4096
#define FILENAME_BUFSIZE 256
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
	size_t data_size;
    uint8_t package_number;
    uint8_t total_packages;
} Header;
