#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>

#define DEFAULT_AS_PORT "58002"
#define DEFAULT_FS_PORT "59002"

char *pdIP = NULL;
char *pdPort = NULL;
char *asIP = NULL;
char *asPort = NULL;

static void parseArgs(long argc, char* const argv]) {
	
}
