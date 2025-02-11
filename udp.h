#include <stdio.h>
#include <stdlib.h>

#define BUFSIZE             2048
#define FILENAME_BUFSIZE    256
#define DATABUFSIZE         (BUFSIZE - sizeof(Header))
#define EXIT                0
#define LS                  1
#define DELETE              2
#define PUT                 3
#define GET                 4

#define FALSE               0
#define TRUE                1

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
