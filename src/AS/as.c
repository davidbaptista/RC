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

#define bool int
#define true 1
#define false 0

#define DEFAULT_AS_PORT "58002"

#define BUFFER_SIZE 512

bool verbose = false;

char *asPort = NULL;

static void parseArgs(long argc, char* const argv[]) {
    char c;

    asPort = DEFAULT_AS_PORT;

    while ((c = getopt(argc, argv, "p:v")) != -1){
        switch (c) {
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

long readMessage(int fd, char *msg) {
	ssize_t nleft, nread, ntotal = 0;
	char *ptr;
	ptr = msg;
	nleft = BUFFER_SIZE;

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
		if(ptr[nread-1] == '\n'){
			break;
		}
		ptr += nread;
	}

	ptr[nread] = '\0';

	return ntotal;
}

int main(int argc, char *argv[]) {
	DIR *d;
	FILE *fp;
	fd_set fds;	
	ssize_t n;
    socklen_t asaddrlen, pdaddrlen;
	struct dirent *dir;
    struct sockaddr_in asaddr, pdaddr;
    struct addrinfo ashints, *asres, pdhints, *pdres;
	char *c;
	char dirname[16];
	char filename[32];
	char buffer[BUFFER_SIZE];
	char arg1[4], arg2[6], arg3[16], arg4[16], arg5[16];
	int i;
	int asfd, pdfd;

	int errcode;

	parseArgs(argc, argv);

	if((asfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket()");
		exit(1);
	}

	if((pdfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket()");
		exit(1);
	}
		
	memset(&ashints, 0, sizeof(ashints));

	ashints.ai_family = AF_INET;
	ashints.ai_socktype = SOCK_DGRAM;
	ashints.ai_flags = AI_PASSIVE;

	if((getaddrinfo(NULL, asPort, &ashints, &asres)) != 0) {
		perror("getaddrinfo()");
		exit(1);
	}

	if(bind(asfd, asres->ai_addr, asres->ai_addrlen) == -1) {
		perror("bind()");
		exit(1);
	}

	while(true) {
		asaddrlen = sizeof(asaddr);

		n = recvfrom(asfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&asaddr, &asaddrlen);

		if(n == -1) {
			perror("recvfrom()");
			exit(1);
		}

		if(verbose) {
			puts(buffer);
		}

		sscanf(buffer, "%s ", arg1);

		if(strcmp(arg1, "REG") == 0) {
			n = sscanf(buffer, "%s %s %s %s %s", arg1, arg2, arg3, arg4, arg5);

			if(n == 5) {
				if(strlen(arg2) == 5 && strlen(arg3) == 8) {
					int valid = true;
					
					// argument validation
					c = arg2;
					while(*c) {
						if(isdigit(*c++) == 0) {
							valid = false;
						}
					}

					c = arg3;
					while(*c) {
						if(isalnum(*c++) == 0) {
							valid = false;
						}
					}

					if(!valid) {
						sendto(asfd, "RRG NOK\n", 8, 0, (struct sockaddr*)&asaddr, asaddrlen);

						if(verbose) {
							puts("RRG NOK");
						}
						continue;
					}

					sprintf(dirname, "AS/USERS/%s", arg2);

					d = opendir(dirname);

					if(!d) {
						if(mkdir(dirname, 0777) != 0) {
							perror("mkdir()");
							exit(1);
						}

						d = opendir(dirname);
					}
					
					if(d) {
						sprintf(filename, "%s/%s_pass.txt", dirname, arg2);

						fp = fopen(filename, "r");

						if(fp == NULL) {
							fp = fopen(filename, "w");

							if(fp == NULL) {
								perror("fopen()");
								exit(1);
							}

							n = fwrite(arg3, 1, strlen(arg3), fp);

							if(n != 8) {
								perror("fwrite()");
								fclose(fp);
								exit(1);
							}

							n = sendto(asfd, "RRG OK\n", 7, 0, (struct sockaddr*)&asaddr, asaddrlen);

							if(n == -1) {
								perror("sendto()");
								exit(1);
							}

							if(verbose) {
								puts("RRG OK");
							}

							fclose(fp);
						}
						else {
							n = fread(buffer, 1, BUFFER_SIZE, fp);
							buffer[n] = '\0';

							if(strcmp(buffer, arg3) == 0) {
								n = sendto(asfd, "RRG OK\n", 7, 0, (struct sockaddr*)&asaddr, asaddrlen);

								if(n == -1) {
									perror("sendto()");
									exit(1);
								}

								if(verbose) {
									puts("RRG OK");
								}
							}
							else {
								n = sendto(asfd, "RRG NOK\n", 8, 0, (struct sockaddr*)&asaddr, asaddrlen);

								if(n == -1) {
									perror("sendto()");
									exit(1);
								}

								if(verbose) {
									puts("RRG NOK");
								}
							}

							fclose(fp);
						}
						sprintf(filename, "%s/%s_reg.txt", dirname, arg2);

						fp = fopen(filename, "w");

						if(fp == NULL) {
							perror("fopen()");
							exit(1);
						}

						sprintf(buffer, "%s %s", arg4, arg5);

						n = fwrite(buffer, 1, strlen(buffer), fp);

						if(n < 0) {
							perror("fwrite()");
							exit(1);
						}

						fclose(fp);
					}
					else {
						perror("opendir()");
						exit(1);
					}					
				}
			}
			else {
				n = sendto(asfd, "RRG ERR\n", 8, 0, (struct sockaddr*)&asaddr, asaddrlen);

				if(n < 0) {
					perror("sendto()");
					exit(1);
				}

				if(verbose) {
					puts("RRG ERR\n");
				}
			}

		}
		else if(strcmp(arg1, "UNR") == 0) {

		}
		else {
			// error
		}
	}

	freeaddrinfo(asres);
	close(asfd);
	exit(0);
}