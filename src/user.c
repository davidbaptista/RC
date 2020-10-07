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

#define DEFAULT_IP "127.0.0.1"


#define bool int
#define true 1
#define false 0


char *asIP = NULL;
char *asPort = NULL;
char *fsIP = NULL;
char *fsPort = NULL;

void parseArgs(long argc, char* const argv[]) {
	char c;

    asIP = DEFAULT_IP;
    asPort = DEFAULT_AS_PORT;
    fsIP = DEFAULT_IP;
    fsPort = DEFAULT_FS_PORT;

    while ((c = getopt(argc, argv, "n:p:m:q:")) != -1){
        switch (c) {
            case 'n':
                asIP = optarg;
                break;
            case 'p':
                asPort = optarg;
                break;
            case 'm':
                fsIP = optarg;
                break;
			case 'q':
                fsPort = optarg;
                break;
        }
    }
}

void writeMessage(int fd, char *msg) {
	ssize_t nbytes, nleft, nwritten;
	char *ptr;

	ptr = msg;
	nleft = sizeof(msg);

	printf("%ld",nleft);

	while(nleft > 0) {
		nwritten = write(fd, ptr, nleft);
		if(nwritten <= 0) {
			exit(1);
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
} 

void readMessage(int fd, char *msg) {
	ssize_t nbytes, nleft, nread;
	char *ptr;

	ptr = msg;
	nleft = sizeof(msg);

	while(nleft > 0) {
		nread = read(fd, ptr, nleft);
		if(nread == -1) {
			exit(1);
		}
		else if(nread == 0) {
			break;
		}
		nleft -= nread;
		ptr += nread;
	}

	ptr[nread] = '\0';
}

int main(int argc, char *argv[]) {
	// connection vars
	struct sockaddr_in addr;
	struct addrinfo ashints, fshints, *asres, *fsres;
	int n;
	int RID;
	int asfd, fsfd;
	char UID[5], pass[8];

	// input vars
	char line[256];
	char command[128];
	char message[512];
	char arg1[128], arg2[128];
	int c;


	parseArgs(argc, argv);

	asfd = socket(AF_INET, SOCK_STREAM, 0); // AS Socket TCP
	fsfd = socket(AF_INET, SOCK_STREAM, 0); // FS Socket TCP


    memset(&ashints, 0, sizeof(ashints));
    ashints.ai_family=AF_INET; // IPv4
    ashints.ai_socktype=SOCK_STREAM; // TDP
	ashints.ai_flags=AI_CANONNAME;

	memset(&fshints, 0, sizeof(fshints));
    fshints.ai_family=AF_INET; // IPv4
    fshints.ai_socktype=SOCK_STREAM; // TDP
	fshints.ai_flags=AI_CANONNAME;


	if(getaddrinfo(asIP, asPort, &ashints, &asres) != 0) {
        exit(1);
    }	

	if(getaddrinfo(fsIP, fsPort, &fshints, &fsres) != 0) {
        exit(1);
	}

	n = connect(asfd, asres->ai_addr, asres->ai_addrlen);
	if(n == -1) {
		exit(1);
	}

	puts("passei");

	while(true) {
		fgets(line, sizeof(line), stdin);

		c = sscanf(line, "%s %s %s", command, arg1, arg2);


		if(strcmp(command, "login") == 0 && c == 3 && strlen(arg1) == 5 && strlen(arg2) == 8) {
			sprintf(message, "LOG %s %s", arg1, arg2);

			writeMessage(asfd, message);
			readMessage(asfd, message);

			puts("cheguei");

			if(strcmp(message, "RLO OK\n")==0) {
				strcpy(UID, arg1);
				strcpy(UID, arg2);
				puts("You are now logged in");
			}
			else {
				puts(message);
			}
		}
		else if(strcmp(command, "req") == 0) {

			RID = rand() % 10000;

			if(strcmp(arg1, "R") == 0 || strcmp(arg1, "U") == 0 || strcmp(arg1, "D") == 0){
				sprintf(message, "REG %s %04d %s %s\n", UID, RID, arg1, arg2);
				writeMessage(asfd, message);	
			}
			else{
				sprintf(message, "REG %s %04d %s\n", UID, RID, arg1);
				writeMessage(asfd, message);
			}

			readMessage(asfd, message);
			puts(message);

		}
		else if(strcmp(command, "val") == 0) {

			sprintf(message, "AUT %s %d %s\n", UID, RID, arg1);
			writeMessage(asfd, message);

			readMessage(asfd, message);
			sscanf(message, "%s %s\n", arg1, arg2);

			if(strcmp(arg2, "0") != 0){
				printf("Authenticated! (TID=%s)\n", arg2);
			}else{
				puts("Authentication Failed!");
			}
		}
		else if(strcmp(command, "list") == 0) {
			/*fs*/
		}
		else if(strcmp(command, "retrieve") == 0) {
			/*fs*/
		}
		else if(strcmp(command, "upload") == 0) {
			
		}
		else if(strcmp(command, "delete") == 0) {
			
		}
		else if(strcmp(command, "remove") == 0) {
			
		}
		else if(strcmp(command, "exit") == 0) {
			close(asfd);
		}
		else {
			// idk
		}
	}

}
