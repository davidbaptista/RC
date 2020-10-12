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

#define DEFAULT_PD_PORT "57002"
#define DEFAULT_AS_PORT "58002"

#define bool int
#define true 1
#define false 0

#define max(A, B) ((A) >= (B)?(A):(B))

char *pdIP = NULL;
char *pdPort = NULL;
char *asIP = NULL;
char *asPort = NULL;

static void parseArgs (long argc, char* const argv[]) {
    char c;

    pdIP = argv[1];
    pdPort = DEFAULT_PD_PORT;
    asIP = argv[1];
    asPort = DEFAULT_AS_PORT;

    while ((c = getopt(argc, argv, "d:n:p:")) != -1){
        switch (c) {
            case 'd':
                pdPort = optarg;
                break;
            case 'n':
                asIP = optarg;
                break;
            case 'p':
                asPort = optarg;
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    int c;
    int counter;
    int asfd, pdfd;
    char msg[128];
    char line[128];
    char buffer[128];
    char command[128];
    char arg1[16], arg2[16];
    char UID[6], pass[9];
    struct sockaddr_in asaddr, pdaddr;
    struct addrinfo ashints, *asres, pdhints, *pdres;
    ssize_t n;
	fd_set fds;	
    socklen_t asaddrlen, pdaddrlen;


    parseArgs(argc, argv);

    asfd = socket(AF_INET,SOCK_DGRAM,0); // UDP socket
    if(asfd == -1) {
        exit(1);
    }

	pdfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(pdfd == -1) {
		exit(1);
	}

    memset(&ashints, 0, sizeof(ashints));
    ashints.ai_family=AF_INET; // IPv4
    ashints.ai_socktype=SOCK_DGRAM; // UDP
	ashints.ai_flags=AI_PASSIVE;

	memset(&pdhints, 0, sizeof(pdhints));
    pdhints.ai_family=AF_INET; // IPv4
    pdhints.ai_socktype=SOCK_DGRAM; // UDP
	pdhints.ai_flags=AI_PASSIVE;

    if(getaddrinfo(asIP, asPort, &ashints, &asres) != 0) {
        exit(1);
	}

	if(getaddrinfo(pdIP, pdPort, &pdhints, &pdres) != 0) {
		exit(1);
	}

	if(bind(pdfd, pdres->ai_addr, pdres->ai_addrlen) == -1) {
        perror("bind()");
		exit(1);
	}

	bool reg = false; // checks whether or not there already is a registered user

    while (true){
		FD_ZERO(&fds);
    	FD_SET(pdfd, &fds);
    	FD_SET(0, &fds);
        counter = select(pdfd + 1, &fds, (fd_set *) NULL, (fd_set *) NULL, (struct timeval *) NULL);

        if (counter <= 0) {
            exit(1);
        }
		
        if(FD_ISSET(pdfd, &fds)){
			pdaddrlen = sizeof(pdaddr);
			n = recvfrom(pdfd, buffer, 128, 0, (struct sockaddr*)&pdaddr, &pdaddrlen);
			buffer[n]= '\0';

            c = sscanf(buffer, "%s %s %s", command, arg1, arg2);

            if(strcmp(arg1,UID)==0){
                strcpy(msg,"RVC OK\n");
                printf("VLC=%s\n",arg2);
            }else{
                strcpy(msg,"RVC NOK\n");
            }

            //puts(msg);
            
            n = sendto(pdfd, msg, strlen(msg), 0, pdres->ai_addr, pdres->ai_addrlen);
            if(n == -1) {
                exit(1);
            }

        }
        if(FD_ISSET(0, &fds)){
            fgets(line, sizeof(line), stdin);

            c = sscanf(line, "%s %s %s", command, arg1, arg2);

            if(c == 1 && strcmp(command, "exit") == 0) {
                if(reg) {
                    sprintf(msg, "UNR %s %s\n", arg1, arg2);

                    n = sendto(asfd, msg, strlen(msg), 0, asres->ai_addr, asres->ai_addrlen);
                    if(n == -1) {
                        exit(1);
                    }

                    asaddrlen = sizeof(asaddr);
                    n = recvfrom(asfd, buffer, 128, 0, (struct sockaddr*)&asaddr, &asaddrlen);
                    buffer[n] = '\0';

                    if(n == -1) {
                        exit(1);
                    }

                    if(!strcmp(buffer, "RUN OK\n")) {
                        reg = false;
                    }

                    write(1, "echo: ", 6);	// temporary
                    write(1, buffer, n);	// temporary
                }
				else {
					puts("User is unregistered");	// temporary
				}
            }
            else if (c == 3 && strcmp(command, "reg") == 0 && strlen(arg1) == 5 && strlen(arg2) == 8) {
                sprintf(msg, "REG %s %s %s %s\n", arg1, arg2, pdIP, pdPort);

                n = sendto(asfd, msg, strlen(msg), 0, asres->ai_addr, asres->ai_addrlen);
                if(n == -1) {
                    exit(1);
                }

                asaddrlen = sizeof(asaddr);
                n = recvfrom(asfd, buffer, 128, 0, (struct sockaddr*)&asaddr, &asaddrlen);
                buffer[n] = '\0';

                if(n == -1) {
                    exit(1);
                }

                if(!strcmp(buffer, "RRG OK\n")) {
                    reg = true;
                    strcpy(UID,arg1);
                    strcpy(pass,arg2);
                }

                write(1, "echo: ", 6); 	// temporary
                write(1, buffer, n);	// temporary
            }
			else {
				puts("Command unknown"); 	// temporary	
			}
        }
    }

    freeaddrinfo(asres);
    close(asfd);
    exit(0);

}