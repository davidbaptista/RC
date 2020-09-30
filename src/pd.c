#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#define DEFAULT_CLIENT_PORT "57002"
#define DEFAULT_SERVER_PORT "58002"

#define bool int
#define true 1
#define false 0

char *clientIP = NULL;
char *clientPort = NULL;
char *serverIP = NULL;
char *serverPort = NULL;

static void parseArgs (long argc, char* const argv[]){

    char c;

    clientIP = argv[1];
    clientPort = DEFAULT_CLIENT_PORT;
    serverIP = argv[1];
    serverPort = DEFAULT_SERVER_PORT;

    while ((c = getopt(argc, argv, "d:n:p:")) != -1){
        switch (c) {
            case 'd':
                clientPort = optarg;
                break;
            case 'n':
                serverIP = optarg;
                break;
            case 'p':
                serverPort = optarg;
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    int fd, errcode;
    char line[128];
    char buffer[128];
    char host[NI_MAXHOST], service[NI_MAXSERV];
    struct sockaddr_in addr;
    struct addrinfo hints, *res, *p;
    ssize_t n;
    socklen_t addrlen;


    parseArgs(argc, argv);

    fd = socket(AF_INET,SOCK_DGRAM,0); // UDP socket
    if(fd == -1) {
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family=AF_INET; // IPv4
    hints.ai_socktype=SOCK_DGRAM; // UDP
	hints.ai_flags=AI_CANONNAME;

    if(getaddrinfo(serverIP, serverPort, &hints, &res) != 0) {
        exit(1);
    }	

	bool reg = false;
    while(fgets(line, sizeof(line), stdin) != NULL) {
        char command[128];
		char msg[128];
        char arg1[16], arg2[16];
        int c;

        c = sscanf(line, "%s %s %s", command, arg1, arg2);

        if(strcmp(command, "exit") == 0) {
			if(reg) {
				sprintf(msg, "UNR %s %s\n", arg1, arg2);

				n = sendto(fd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen);
				if(n == -1) {
					exit(1);
				}

				addrlen = sizeof(addr);
				n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
				buffer[n] = '\0';

				if(n == -1) {
					exit(1);
				}

				if(!strcmp(buffer, "RUN OK\n")) {
					reg = false;
				}

				write(1, "echo: ", 6); 
				write(1, buffer, n);
			}

            break;
        }
        else if (strcmp(command, "reg") == 0 && c == 3 && strlen(arg1) == 5 && strlen(arg2) == 8) {
			sprintf(msg, "REG %s %s %s %s\n", arg1, arg2, clientIP, clientPort);

			n = sendto(fd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen);
			if(n == -1) {
				exit(1);
			}

			addrlen = sizeof(addr);
			n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
			buffer[n] = '\0';

			if(n == -1) {
				exit(1);
			}

			if(!strcmp(buffer, "RRG OK\n")) {
				reg = true;
			}
			printf("bool: %d\n", reg);

			write(1, "echo: ", 6); 
			write(1, buffer, n);
        }
    }

    freeaddrinfo(res);
    close(fd);
    exit(0);

}