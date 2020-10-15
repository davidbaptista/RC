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

#define MESSAGE_SIZE 512
#define MAX_OPERATIONS 64

#define bool int
#define true 1
#define false 0

char *asIP = NULL;
char *asPort = NULL;
char *fsIP = NULL;
char *fsPort = NULL;

struct operation{
	char Fname[25];
	char TID[5];
	char Fop;
	bool available;
} operation_default= {.available=1};

void parseArgs(long argc, char* const argv[]) {
	char c;

    asIP = DEFAULT_IP;
    asPort = DEFAULT_AS_PORT;
    fsIP = DEFAULT_IP;
    fsPort = DEFAULT_FS_PORT;

    while ((c = getopt(argc, argv, "n:p:m:q:")) != -1) {
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
	nleft = strlen(msg);

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
	nleft = MESSAGE_SIZE;

	while(nleft > 0) {
		nread = read(fd, ptr, nleft);

		if(nread == -1) {
			exit(1);
		}
		else if(nread == 0) {
			break;
		}

		nleft -= nread;
		if(ptr[nread-1] == '\n'){
			break;
		}
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
	char UID[6], pass[9];

	// input vars
	char line[256];
	char command[128];
	char message[MESSAGE_SIZE];
	char arg1[128], arg2[128];
	char Fname[25];
	char Fop;
	int c;

	int oi = -1; // -1 has a index between req and val. -1 otherwise
	struct operation operations[MAX_OPERATIONS];

	parseArgs(argc, argv);

	asfd = socket(AF_INET, SOCK_STREAM, 0); // AS Socket TCP

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

	while(true) {
		fgets(line, sizeof(line), stdin);

		c = sscanf(line, "%s %s %s", command, arg1, arg2);


		if(strcmp(command, "login") == 0 && c == 3 && strlen(arg1) == 5 && strlen(arg2) == 8) {
			sprintf(message, "LOG %s %s\n", arg1, arg2);

			writeMessage(asfd, message);
			readMessage(asfd, message);

			if(strcmp(message, "RLO OK\n")==0) {
				strcpy(UID, arg1);
				strcpy(pass, arg2);
				puts("You are now logged in");
			}
			else {
				puts("Login failed!");
			}
		}
		else if(strcmp(command, "req") == 0) {
			RID = rand() % 10000;

			if(strcmp(arg1, "R") == 0 || strcmp(arg1, "U") == 0 || strcmp(arg1, "D") == 0){
				sprintf(message, "REQ %s %04d %s %s\n", UID, RID, arg1, arg2);
				writeMessage(asfd, message);
			}
			else {
				sprintf(message, "REQ %s %04d %s\n", UID, RID, arg1);
				writeMessage(asfd, message);
			}

			readMessage(asfd, message);

			if(strcmp(message, "RRQ OK\n") == 0) {
				Fop = arg1[0];
				if(Fop == 'R' || Fop == 'U' || Fop == 'D') {
					strcpy(Fname, arg2);
				}
			}
			else {
				puts(message);
			}
		}
		else if(strcmp(command, "val") == 0) {
			sprintf(message, "AUT %s %04d %s\n", UID, RID, arg1);
			writeMessage(asfd, message);

			readMessage(asfd, message);
			sscanf(message, "%s %s", arg1, arg2);


			if(strcmp(arg2, "0") != 0){
				for(int i = 0; i < MAX_OPERATIONS; i++) {
					if(operations[i].available) {
						operations[i].available = false;
						operations[i].Fop = Fop;
						strcpy(operations[i].TID, arg2);
						if(Fop == 'R' || Fop == 'U' || Fop == 'D') {
							strcpy(operations[i].Fname, Fname);
						}
						break;
					}
				} 

				printf("Authenticated! (TID=%s)\n", arg2);		
			}
			else{
				puts("Authentication Failed!");
			}
		}
		else if(strcmp(command, "list") == 0 || strcmp(command, "l") == 0) {
			int i = 0;
			fsfd = socket(AF_INET, SOCK_STREAM, 0);


			n = connect(fsfd, fsres->ai_addr, fsres->ai_addrlen);
			if(n == -1) {
				exit(1);
			}

			for(i = 0; i < MAX_OPERATIONS; i++) {
				if(operations[i].Fop == 'L') {
					break;
				}
			}

			sprintf(message, "LST %s %s\n", UID, operations[i].TID);
			operations[i].available = true;

			writeMessage(fsfd, message);
			readMessage(fsfd, message);
			i = 1;
			int nfiles;
			char *p;
			p = strtok(message, " ");
			p = strtok(NULL, " ");
			nfiles = strtol(p, NULL, 10);
			p = strtok(NULL, " ");

			for(p; p != NULL; p = strtok(NULL, " ")) {
				printf("%d - ", i);
				printf("%s ", p);
				p = strtok(NULL, " ");
				if(i == nfiles) {
					printf("%s", p);
				}
				else {
					printf("%s \n", p);
				}
				i++;
			}

			close(fsfd);
		}
		else if(strcmp(command, "retrieve") == 0) {
			/*fs*/
		}
		else if(strcmp(command, "upload") == 0 || strcmp(command, "u") == 0) {
			int i = 0;
			fsfd = socket(AF_INET, SOCK_STREAM, 0);

			n = connect(fsfd, fsres->ai_addr, fsres->ai_addrlen);
			if(n == -1) {
				exit(1);
			}

			for(i = 0; i < MAX_OPERATIONS; i++) {
				if(strcmp(operations[i].Fname, arg1) == 0 && operations[i].Fop == 'U') {
					break;
				}
			}

			sprintf(message, "UPL %s %s %s %d %s\n", UID, operations[i].TID, operations[i].Fname, 6, "Hello!");
			puts(message);
			writeMessage(fsfd, message);
			readMessage(fsfd, message);


			if(strcmp(message,"RUP OK\n") == 0) {
				printf("Success uploading %s\n",operations[i].Fname);
			}
			else if(strcmp(message,"RUP DUP\n") == 0) {
				printf("File %s already exists\n",operations[i].Fname);
			}
			else if(strcmp(message,"RUP FULL\n") == 0) {
				printf("File limit exceeded\n");
			}
			else {
				puts(message);
				puts("Upload failed");
			}

			operations[i].available = 1;

			close(fsfd);
		
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

    freeaddrinfo(asres);
	freeaddrinfo(fsres);
	close(asfd);
	exit(0);
}
