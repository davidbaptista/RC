#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <iostream>



//good being global? 
char *myID = NULL;
char *myPort = NULL;
const char *serverID = NULL;
const char *serverPort = NULL;


static void parseArgs (long argc, char* const argv[]){

    switch (argc)
    {
    case 2:
        myID = argv[1];
        break;
    
    case 4:
        myID = argv[1];
        myPort = argv[3];
        break;

    case 6:
        myID = argv[1];
        myPort = argv[3];
        serverID = argv[5];
        break;
    
    case 8:
        myID = argv[1];
        myPort = argv[3];
        serverID = argv[5];
        serverPort = argv[7];
        break;


    default:
        //formato incorreto smth happens
        break;
    }


}

int main(int argc, char * argv []){

    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints,*res;
    struct sockaddr_in addr;
    char buffer[128];
    char host[NI_MAXHOST],service[NI_MAXSERV];
    std::string line;

    parseArgs(argc,argv);

    //implements UDP connection with AS

    fd=socket(AF_INET,SOCK_DGRAM,0);//UDP socket
    if(fd==-1)/*error*/exit(1);

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;//IPv4
    hints.ai_socktype=SOCK_DGRAM;//UDP socket

    /*std::cout << serverID << "\n";
    std::cout << serverPort << "\n";*/

    errcode=getaddrinfo(serverID,serverPort,&hints,&res);
    if(errcode!=0) exit(1);

    std::getline(std::cin, line);

    if(line.compare("exit")==0)
        //is exit
    else{

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