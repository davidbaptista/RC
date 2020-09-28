#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define DEFAULT_CLIENT_PORT "57002"
#define DEFAULT_SERVER_PORT "58002"

char *clientIP = NULL;
char *clientPort = NULL;
char *serverIP = NULL;
char *serverPort = NULL;

static void parseArgs(long argc, char* const argv[]) {
    switch (argc) {
    case 2:
        clientIP = argv[1];
        clientPort = DEFAULT_CLIENT_PORT;
        serverIP = clientIP;
        serverPort = DEFAULT_SERVER_PORT;
        break;
    case 4:
        clientIP = argv[1];
        clientPort = argv[3];
        serverIP = clientIP;
        serverPort = DEFAULT_SERVER_PORT;
        break;
    case 6:
        clientIP = argv[1];
        clientPort = argv[3];
        serverIP = argv[5];
        serverPort = DEFAULT_SERVER_PORT;
        break;
    case 8:
        clientIP = argv[1];
        clientPort = argv[3];
        serverIP = argv[5];
        serverPort = argv[7];
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[]) {
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints,*res;
    struct sockaddr_in addr;
    char buffer[128];
    char host[NI_MAXHOST], service[NI_MAXSERV];
    char line[128];

    parseArgs(argc, argv);

    fd = socket(AF_INET,SOCK_DGRAM,0); //UDP socket
    if(fd == -1) {
        exit(1);
    }

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; //IPv4
    hints.ai_socktype=SOCK_DGRAM; //UDP

    if(getaddrinfo(serverIP, serverPort, &hints,&res) != 0) {
        exit(1);
    }

    while(fgets(line, sizeof(line), stdin) != NULL) {
        char command[128];
        char arg1[16], arg2[16];
        int c;

        c = sscanf(line, "%s %s %s", command, arg1, arg2);

        if(strcmp(command, "exit") == 0) {
            break;
        }
        else if (strcmp(command, "reg") == 0 && c == 3) {

        }
    }

    /*n=sendto(fd,"Hello!\n",7,0,res->ai_addr,res->ai_addrlen);
    if(n==-1) exit(1);

    addrlen=sizeof(addr);
    n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
    if(n==-1) exit(1);

    write(1,"echo: ",6); write(1,buffer,n);
    write(1,buffer,n);*/

    freeaddrinfo(res);

    close(fd);
    exit(0);

}