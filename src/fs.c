#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <dirent.h>

#pragma GCC diagnostic ignored "-Wformat-overflow"

#define DEFAULT_AS_PORT "58002"
#define DEFAULT_FS_PORT "59002"

#define BUFFER_SIZE 1024
#define AUX_SIZE 600
#define COMMAND_SIZE 4
#define UID_SIZE 6
#define TID_SIZE 5
#define FNAME_SIZE 25

#define PROTOCOL_ERROR_MESSAGE "Error: Command not supported"

#define bool int
#define true 1
#define false 0

#define max(A, B) ((A) >= (B)?(A):(B))

char *fsPort = NULL;
char *asIP = NULL;
char *asPort = NULL;

bool verbose = false;

static void parseArgs(long argc, char* const argv[]) {
    char c;

    fsPort = DEFAULT_FS_PORT;
    asIP = argv[1];
    asPort = DEFAULT_AS_PORT;

    while ((c = getopt(argc, argv, "q:n:p:v")) != -1){
        switch (c) {
            case 'q':
                fsPort = optarg;
                break;
            case 'n':
                asIP = optarg;
                break;
            case 'p':
                asPort = optarg;
                break;
			case 'v':
				verbose = true;
				break;
        }
    }
}

long writeMessage(int fd, char *msg, long int msgSize) {
	ssize_t nleft, nwritten, ntotal = 0;
	char *ptr;
	ptr = msg;
	nleft = msgSize;

	while(nleft > 0) {
		nwritten = write(fd, ptr, nleft);

		if(nwritten <= 0) {
			exit(1);
		}
		ntotal += nwritten;
		nleft -= nwritten;
		ptr += nwritten;
	}

	return ntotal;
} 

long readMessage(int fd, char *msg, long int msgSize) {
	ssize_t nleft, nread, ntotal = 0;
	char *ptr;
	ptr = msg;
	nleft = msgSize;

	while(nleft > 0) {
		nread = read(fd, ptr, nleft);

		if(nread == -1) {
			exit(1);
		}
		else if(nread == 0) {
			break;
		}

		nleft -= nread;
		ntotal += nread;
		if(ptr[nread-1] == '\n') {
			break;
		}
		ptr += nread;
	}

	ptr[nread] = '\0';

	return ntotal;
}

int main(int argc, char *argv[]) {
    int asfd, fsfd, newfd;
	char UID[UID_SIZE];
	char TID[TID_SIZE];
	char buffer[BUFFER_SIZE];
	char command[COMMAND_SIZE];
	struct sigaction act;
    struct sockaddr_in asaddr, fsaddr;
    struct addrinfo ashints, *asres, fshints, *fsres;
    socklen_t asaddrlen, fsaddrlen;
	act.sa_handler = SIG_IGN;
	pid_t pid;
	ssize_t n;
	int ret;

    parseArgs(argc, argv);

	if(sigaction(SIGCHLD, &act, NULL) == -1) {
		exit(1);
	}

    asfd = socket(AF_INET,SOCK_DGRAM,0); // UDP socket
    if(asfd == -1) {
        exit(1);
    }

	fsfd = socket(AF_INET, SOCK_STREAM, 0);
	if(fsfd == -1) {
		exit(1);
	}

    memset(&ashints, 0, sizeof(ashints));
    ashints.ai_family=AF_INET; // IPv4
    ashints.ai_socktype=SOCK_DGRAM; // UDP
	ashints.ai_flags=AI_PASSIVE;

	memset(&fshints, 0, sizeof(fshints));
    fshints.ai_family=AF_INET; // IPv4
    fshints.ai_socktype=SOCK_STREAM; // TCP
	fshints.ai_flags=AI_PASSIVE;

    if(getaddrinfo(asIP, asPort, &ashints, &asres) != 0) {
		perror("getaddrinfo()");
        exit(1);
	}

	if(getaddrinfo(NULL, fsPort, &fshints, &fsres) != 0) {
		perror("getaddrinfo()");
		exit(1);
	}

	if(bind(fsfd, fsres->ai_addr, fsres->ai_addrlen) == -1) {
        perror("bind()");
		exit(1);
	}

	if(listen(fsfd, 128) == -1) {
		perror("listen()");
		exit(1);
	}

    while (true) {
		fsaddrlen = sizeof(fsaddrlen);

		do {
			newfd = accept(fsfd, (struct sockaddr*)&fsaddr, &fsaddrlen);
		} while(newfd == -1 && errno == EINTR);

		if(newfd == -1) {
			perror("accept()");
			exit(1);
		}

		if((pid=fork()) == -1) {
			perror("fork()");
			exit(1);
		}
		else if(pid == 0) {
			close(fsfd);

			char Fop;
			char FName[11];
			while(readMessage(newfd, buffer, (long)14)) {
				sscanf(buffer, "%s %s %s", command, UID, TID);
				
				if(strlen(UID) == 5 && strlen(TID) == 4) {
					if(strcmp(command, "LST") == 0) {
						strcpy(command, "RLS");
					}
					else if(strcmp(command, "RTV") == 0) {
						strcpy(command, "RRT");
					}
					else if(strcmp(command, "UPL") == 0) {
						strcpy(command, "RUP");
					}
					else if(strcmp(command, "DEL") == 0) {
						strcpy(command, "RDL");
					}
					else if(strcmp(command, "REM") == 0) {
						strcpy(command, "RRM");
					}
					else {
						writeMessage(newfd, "ERR\n", 4);
						break;
					}

					sprintf(buffer, "VLD %s %s\n", UID, TID);
					
					n = sendto(asfd, buffer, strlen(buffer), 0, asres->ai_addr, asres->ai_addrlen);
					if(verbose) {
						puts(buffer);
					}

					if(n < 0) {
						puts("sendto error");
						exit(1);
					}

					n = recvfrom(asfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&asaddr, &asaddrlen);

					if(n < 0) {
						puts("recvfrom error");
						exit(1);
					}

					buffer[n] = '\0';

					if(verbose) {
						puts(buffer);
					}

					sscanf(buffer, "CNF %s %s %c %s", UID, TID, &Fop, FName);

					DIR *d;
					FILE *fp;
					struct dirent *dir;
					char aux[AUX_SIZE];
					char dirname[12];
					long size;
					int dup = 0;
					int i = 0;
					char *ptr;
					ssize_t nbytes;
					ssize_t nread;

					sprintf(dirname, "USERS/%s/", UID);

					switch(Fop) {
						case 'L':
							d = opendir(dirname);
							
							if(d) {
								i = 0;
								strcpy(aux, "");
								while((dir = readdir(d)) != NULL) {
									i++;
									if(strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
										i--;
										continue;
									}
									sprintf(buffer, "%s/%s", dirname, dir->d_name);
									fp = fopen(buffer, "rb"); // FIX LATER

									if(fp == NULL) {
										perror("fopen()");
										exit(1);
									}

									if(fseek(fp, 0, SEEK_END) < 0) {
										perror("fseek()");
										exit(1);
									}
									
									size = ftell(fp);

									if(size < 0) {
										perror("ftell()");
										exit(1);
									}
									if(fseek(fp, 0, SEEK_SET) < 0) {
										perror("fseek()");
										exit(1);
									}

									if(fclose(fp) < 0) {
										perror("fclose()");
										exit(1);
									}

									if(strlen(dir->d_name) > 24) {
										printf("Error: filename too big %s", dir->d_name);
										exit(1);
									}
									sprintf(buffer, " %s %ld", dir->d_name, size);
									strcat(aux, buffer);
								}

								if(i > 0) {
									sprintf(buffer, "RLS %d%s\n", i, aux);
								}
								else {
									sprintf(buffer, "RLS EOF\n");
								}
							}
							else {
								sprintf(buffer, "RLS EOF\n");
							}
							writeMessage(newfd, buffer, strlen(buffer));

							if(verbose) {
								puts(buffer);
							}

							break;
						case 'R':
							d = opendir(dirname);
							fp = NULL;

							if(d) {
								sprintf(buffer, "%s/%s", dirname, FName);
								fp = fopen(buffer, "rb");

								if(fp == NULL) {
									sprintf(buffer, "RRT EOF\n");
									writeMessage(newfd, buffer, strlen(buffer));
									break;
								}

								if(fseek(fp, 0, SEEK_END) < 0) {
									perror("fseek()");
									exit(1);
								}

								size = ftell(fp);

								if(size < 0) {
									perror("ftell()");
									exit(1);
								}

								if(fseek(fp, 0, SEEK_SET) < 0) {
									perror("fseek()");
									exit(1);
								}

								sprintf(buffer, "RRT OK %ld ", size);

								writeMessage(newfd, buffer, strlen(buffer));

								nbytes = 0;
			
								while(nbytes < size) {
									fread(buffer, 1, BUFFER_SIZE, fp);
									nbytes += writeMessage(newfd, buffer, BUFFER_SIZE);
									bzero(buffer, BUFFER_SIZE);
								}

								writeMessage(newfd, "\n", 1);

								if(fclose(fp) < 0) {
									perror("fclose()");
									exit(1);
								}
							}
							else {
								sprintf(buffer, "RRT NOK\n");
								writeMessage(newfd, buffer, strlen(buffer));
							}
							break;
						case 'D':
							d = opendir(dirname);
							fp = NULL;

							if(d) {
								sprintf(buffer, "%s/%s", dirname, FName);

								if(remove(buffer) == 0){
									writeMessage(newfd, "RDL OK\n", (long) 7);
								}
								else{
									writeMessage(newfd, "RDL EOF\n", (long) 8);
								}
							}
							else{
								writeMessage(newfd, "RDL NOK\n", (long) 8);
							}
							break;
						case 'U':
							i = 0; 
							dup = 0;
							d = opendir(dirname);

							if(!d){
								mkdir(dirname, 0777);
								d = opendir(dirname);
							}

							while((dir = readdir(d)) != NULL) {
								if(strcmp(dir->d_name, FName) == 0){
									dup = 1;
									break;
								}
								i++;
							}

							if(dup == 1) {
								writeMessage(newfd, "RUP DUP\n", (long)8);
								break;
							}

							if(i >= 15){
								writeMessage(newfd, "RUP FULL\n", (long)9);
								break;
							}
							
							sprintf(buffer, "%s/%s", dirname, FName);

							fp = fopen(buffer, "wb");

							if(fp == NULL) {
								perror("fopen()");
								exit(1);
							}

							char fsize[11];

							ptr = fsize;
							nbytes = 0;

							while(1) {
								nread = read(newfd, ptr, 1);

								if(nread == -1) {
									exit(1);
								}
								else if(nread == 0) {
									break;
								}
								nbytes += nread;

								if(ptr[nread-1] == ' ') {
									break;
								}
								ptr += nread;
							}

							fsize[nbytes] = '\0';

							size = strtol(fsize, NULL, 10);
							nbytes = 0;

							while(nbytes < size) {
								nread = read(newfd, buffer, BUFFER_SIZE);
								nbytes += nread;

								if(fwrite(buffer, 1, nread, fp) < 0) {
									exit(1);
								}

								bzero(buffer, BUFFER_SIZE);
							}

							break;
						case 'X':
							d = opendir(dirname);

							if(d) {
								bool ok = true;
								while((dir = readdir(d)) != NULL) {
									sprintf(buffer, "%s/%s", dirname, dir->d_name);

									if(remove(buffer) != 0) {
										ok = false;
									}
								}

								if(rmdir(dirname) != 0) {
									perror("rmdir()");
									exit(1);
								}

								if(ok) {
									writeMessage(newfd, "REM OK\n", (long) 7);
								}

							}
							else {
								writeMessage(newfd, "RRM NOK\n", (long) 8);
							}
							break;
						case 'E':
							sprintf(buffer, "%s INV\n", command);
							break;
						default:
							sprintf(buffer, "%s ERR\n", command);
							break;
					}
					
					close(newfd);
					exit(0);
				}
			}
		}
		do {
			ret = close(newfd);
		} while(ret == -1 && errno == EINTR);

		if(ret == -1) {
			perror("close()");
			exit(1);
		}
    }

    freeaddrinfo(asres);
	freeaddrinfo(fsres);
    close(asfd);
	close(fsfd);
    exit(0);
}