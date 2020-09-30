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

#define DEFAULT_CLIENT_PORT "57002"
#define DEFAULT_SERVER_PORT "58002"

#define bool int
#define true 1
#define false 0

#define max(A, B) ((A) >= (B)?(A):(B))

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
    int c;
    int counter;
    int serverfd, inputfd = 0;
    char msg[128];
    char line[128];
    char buffer[128];
    char command[128];
    char arg1[16], arg2[16];
    struct sockaddr_in addr;
    struct addrinfo hints, *res;
    ssize_t n;
	fd_set fds;	
    socklen_t addrlen;


    parseArgs(argc, argv);

    serverfd = socket(AF_INET,SOCK_DGRAM,0); // UDP socket
    if(serverfd == -1) {
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family=AF_INET; // IPv4
    hints.ai_socktype=SOCK_DGRAM; // UDP
	hints.ai_flags=AI_CANONNAME;

    if(getaddrinfo(serverIP, serverPort, &hints, &res) != 0) {
        exit(1);
    }	

	bool reg = false; // checks whether or not there already is a registered user

    FD_ZERO(&fds);
    FD_SET(serverfd, &fds);
    FD_SET(inputfd, &fds);

    while (true){
        counter = select(serverfd, &fds, (fd_set *) NULL, (fd_set *) NULL, (struct timeval *) NULL);

        if (counter <= 0) {
            exit(1);
        }

        if(FD_ISSET(inputfd, &fds)){
            fgets(line, sizeof(line), stdin);

            c = sscanf(line, "%s %s %s", command, arg1, arg2);

            if(strcmp(command, "exit") == 0) {
                if(reg) {
                    sprintf(msg, "UNR %s %s\n", arg1, arg2);

                    n = sendto(serverfd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen);
                    if(n == -1) {
                        exit(1);
                    }

                    addrlen = sizeof(addr);
                    n = recvfrom(serverfd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
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
            }
            else if (strcmp(command, "reg") == 0 && c == 3 && strlen(arg1) == 5 && strlen(arg2) == 8) {
                sprintf(msg, "REG %s %s %s %s\n", arg1, arg2, clientIP, clientPort);

                n = sendto(serverfd, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen);
                if(n == -1) {
                    exit(1);
                }

                addrlen = sizeof(addr);
                n = recvfrom(serverfd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
                buffer[n] = '\0';

                if(n == -1) {
                    exit(1);
                }

                if(!strcmp(buffer, "RRG OK\n")) {
                    reg = true;
                }

                write(1, "echo: ", 6); 
                write(1, buffer, n);
            }
        }
        if(FD_ISSET(serverfd, &fds)){

        }
    }

    freeaddrinfo(res);
    close(serverfd);
    exit(0);

}