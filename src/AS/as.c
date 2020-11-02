#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <dirent.h>

#define max(A, B) ((A) >= (B)?(A):(B))

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
	pid_t pid;
    socklen_t asudpaddrlen, pdaddrlen, astcpaddrlen;
	struct dirent *dir;
	struct sigaction act;
    struct sockaddr_in asudpaddr, pdaddr, astcpaddr;
    struct addrinfo asudphints, *asudpres, pdhints, *pdres, astcphints, *astcpres;
	char *c;
	char dirname[16];
	char filename[32];
	char buffer[BUFFER_SIZE];
	char arg1[4], arg2[6], arg3[16], arg4[16], arg5[16];
	int i;
	int ret;
	int counter;
	int asudpfd, pdfd, astcpfd, newfd;

	act.sa_handler = SIG_IGN;

	parseArgs(argc, argv);

	if(sigaction(SIGCHLD, &act, NULL) == -1) {
		exit(1);
	}

	if((asudpfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket()");
		exit(1);
	}

	if((pdfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket()");
		exit(1);
	}

	if((astcpfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket()");
		exit(1);
	}
		
	memset(&asudphints, 0, sizeof(asudphints));
	asudphints.ai_family = AF_INET;
	asudphints.ai_socktype = SOCK_DGRAM;
	asudphints.ai_flags = AI_PASSIVE;

	memset(&pdhints, 0, sizeof(pdhints));
    pdhints.ai_family=AF_INET;
    pdhints.ai_socktype=SOCK_DGRAM; 

	memset(&astcphints, 0, sizeof(astcphints));
    astcphints.ai_family=AF_INET;
    astcphints.ai_socktype=SOCK_STREAM; 
	astcphints.ai_flags=AI_PASSIVE;

	if((getaddrinfo(NULL, asPort, &asudphints, &asudpres)) != 0) {
		perror("getaddrinfo()");
		exit(1);
	}

	if((getaddrinfo(NULL, asPort, &astcphints, &astcpres)) != 0) {
		perror("getaddrinfo()");
		exit(1);
	}

	if(bind(asudpfd, asudpres->ai_addr, asudpres->ai_addrlen) == -1) {
		perror("bind()");
		exit(1);
	}

	if(bind(astcpfd, astcpres->ai_addr, astcpres->ai_addrlen) == -1) {
		perror("bind()");
		exit(1);
	}

	if(listen(astcpfd, 128) == -1) {
		perror("listen()");
		exit(1);
	}

	while(true) {
		FD_ZERO(&fds);
		FD_SET(asudpfd, &fds);
		FD_SET(astcpfd, &fds);

		counter = select(max(asudpfd, astcpfd) + 1, &fds, (fd_set *) NULL, (fd_set *)NULL, (struct timeval *) NULL);
 		if(counter <= 0) {
            exit(1);
        }

		if(FD_ISSET(asudpfd, &fds)) {
			asudpaddrlen = sizeof(asudpaddr);

			n = recvfrom(asudpfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&asudpaddr, &asudpaddrlen);

			buffer[n] = '\0';

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
								break;
							}
						}

						c = arg3;
						while(*c) {
							if(isalnum(*c++) == 0) {
								valid = false;
								break;
							}
						}

						if(!valid) {
							sendto(asudpfd, "RRG NOK\n", 8, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

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

							//first time user registration
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

								n = sendto(asudpfd, "RRG OK\n", 7, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

								if(n == -1) {
									perror("sendto()");
									exit(1);
								}

								if(verbose) {
									puts("RRG OK");
								}

								fclose(fp);
							}
							//user already registrated
							else {
								n = fread(buffer, 1, BUFFER_SIZE, fp);
								buffer[n] = '\0';

								//verifies user's credentials
								if(strcmp(buffer, arg3) == 0) {
									n = sendto(asudpfd, "RRG OK\n", 7, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

									if(n == -1) {
										perror("sendto()");
										exit(1);
									}

									if(verbose) {
										puts("RRG OK");
									}
								}
								else {
									n = sendto(asudpfd, "RRG NOK\n", 8, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

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
					n = sendto(asudpfd, "RRG ERR\n", 8, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

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
				n = sscanf(buffer, "%s %s %s", arg1, arg2, arg3);

				if(n==3){
					sprintf(dirname, "AS/USERS/%s", arg2);

					d = opendir(dirname);

					//user doesnt exist
					if(!d) {
						sendto(asudpfd, "RUN NOK\n", 8, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);
						if(n < 0) {
							perror("sendto()");
							exit(1);
						}

						if(verbose) {
							puts("RUN NOK\n");
						}
					}
					//user exists
					else{
						sprintf(filename, "%s/%s_pass.txt", dirname, arg2);

						fp = fopen(filename, "r");

						if(fp == NULL) {
							perror("fopen()");
							exit(1);
						}

						n = fread(buffer, 1, BUFFER_SIZE, fp);
						buffer[n] = '\0';

						//verifies user's credentials
						if(strcmp(buffer, arg3) == 0) {
							bool ok = true;

							sprintf(buffer, "%s/%s_reg.txt", dirname, arg2);
							if(remove(buffer) != 0) {
								ok = false;
							}			

							if(ok) {
								n = sendto(asudpfd, "RUN OK\n", 7, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

								if(n < 0) {
									perror("sendto()");
									exit(1);
								}

								if(verbose) {
									puts("RUN OK\n");
								}
							}
							else {
								sendto(asudpfd, "RUN NOK\n", 8, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

								if(n < 0) {
									perror("sendto()");
									exit(1);
								}

								if(verbose) {
									puts("RUN NOK\n");
								}
							}
						}
						//wrong pass
						else{
							sendto(asudpfd, "RUN NOK\n", 8, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

							if(n < 0) {
								perror("sendto()");
								exit(1);
							}

							if(verbose) {
								puts("RUN NOK\n");
							}
						}
					}
				}
				else{
					n = sendto(asudpfd, "RUN ERR\n", 8, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

					if(n < 0) {
						perror("sendto()");
						exit(1);
					}

					if(verbose) {
						puts("RUN ERR\n");
					}
				}
			}
			else {
				n = sendto(asudpfd, "ERR\n", 4, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

				if(n < 0) {
					perror("sendto()");
					exit(1);
				}

				if(verbose) {
					puts("ERR\n");
				}		
			}
		}
		if(FD_ISSET(astcpfd, &fds)) {
			astcpaddrlen = sizeof(astcpaddrlen);

			do {
				newfd = accept(astcpfd, (struct sockaddr*)&astcpaddr, &astcpaddrlen);
			} while (newfd == -1 && errno == EINTR);			

			if(newfd == -1) {
				perror("accept()");
				exit(1);
			}

			if((pid=fork()) == -1) {
				perror("fork()");
				exit(1);
			}
			else if(pid == 0) {
				close(astcpfd);

				readMessage(newfd, buffer);

				sscanf(buffer, "%s ", arg1);

				if(strcmp(arg1, "LOG") == 0) {
					ret = sscanf(buffer, "LOG %s %s\n", arg2, arg3);

					if(ret != 2 || strlen(arg2) != 5 || strlen(arg3) != 8) {
						writeMessage(newfd, "RLO ERR\n", 8);
						close(newfd);
						exit(1);
					}

					sprintf(dirname, "AS/USERS/%s", arg2);

					d = opendir(dirname);

					if(!d) {
						writeMessage(newfd, "RLO ERR\n", 8);
						close(newfd);
						exit(1);
					}

					sprintf(filename, "%s/%s_reg.txt", dirname, arg2);

					fp = fopen(filename, "r");

					if(fp == NULL) {
						writeMessage(newfd, "RLO ERR\n", 8);
						close(newfd);
						exit(1);
					}

					if(fclose(fp) != 0) {
						perror("fclose()");
						close(newfd);
						exit(1);
					}

					sprintf(filename, "%s/%s_pass.txt", dirname, arg2);

					fp = fopen(filename, "r");

					if(fp == NULL) {
						writeMessage(newfd, "RLO ERR\n", 8);
						close(newfd);
						exit(1);
					}

					n = fread(buffer, 1, BUFFER_SIZE, fp);
					buffer[n] = '\0';

					if(strcmp(buffer, arg3) == 0) {
						if(fclose(fp) == NULL) {
							perror("fclose()");
							exit(1);
						}
						sprintf(filename, "%s/%s_login.txt", dirname, arg2);
						if((fp = fopen(filename, "w")) == NULL) {
							perror("fopen()");
							exit(1);
						}
						if(fclose(fp) == NULL) {
							perror("fclose()");
							exit(1);
						}
						writeMessage(newfd, "RLO OK\n", 7);
					}
					else {
						writeMessage(newfd, "RLO NOK\n", 8);
					}

					fclose(fp);
				}
				else if(strcmp(arg1, "REQ") == 0) {
					ret = sscanf(buffer, "REQ %s %s %s %s\n", arg1, arg2, arg3, arg4);


					if(strlen(arg1) != 5 || strlen(arg2) != 4){
						writeMessage(newfd, "RRQ ERR\n", 8);
						exit(0);
					}

					if(!userExists(arg1)) {
						writeMessage(newfd, "RRQ EUSER\n", 10);
						exit(0);		
					}

					if(!userIsLoggedIn(arg1)){
						writeMessage(newfd, "RRQ ELOG\n", 9);
						exit(0);
					}

					if(strcmp(arg3, "X") == 0 || !strcmp(arg3, "U") == 0 || strcmp(arg3, "R") == 0 || !strcmp(arg3, "U") == 0 || !strcmp(arg3, "D") == 0){

						char message[BUFFER_SIZE];
						if((strcmp(arg3, "R") == 0 || !strcmp(arg3, "U") == 0 || !strcmp(arg3, "D") == 0) && ret == 4){
							if(strlen(arg4) <= 24){
								sprintf(message, "VLC %s %04d %s %s", arg1, VC, arg3, arg4);
							}
							else {
								writeMessage(newfd, "RRQ ERR\n", 8);
								exit(0);
							}
						}
						else {
							sprintf(message, "VLC %s %04d %s", arg1, VC, arg3);
						}

						sprintf(filename, "AS/USERS/%s/%s_reg.txt", arg1, arg1);
							
						if((fp = fopen(filename, "r")) === NULL){
							perror("fopen()");
							exit(1);
						}

						n = fread(buffer, 1, BUFFER_SIZE, fp);
						buffer[n] = '\0';

						char pdIP[16];
						char pdPort[6];
						sscanf(buffer, "%s %s", pdIP, pdPort);

						if((getaddrinfo(pdIP, pdPort, &pdhints, &pdres)) != 0) {
							perror("getaddrinfo()");
							exit(1);
						}

						int VC = rand() % 10000;

						n = sendto(pdfd, message, strlen(message), 0, pdres->ai_addr, pdres->ai_addrlen);
						if(n == -1) {
							exit(1);
						}

						n = recvfrom(pdfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&pdaddr, &pdaddrlen);
						buffer[n]= '\0';

						sscanf(buffer, "RVC %s %s", arg1, arg2);
						if(strcmp(arg2, "OK") == 0){
							writeMessage(newfd, "RRQ OK\n", 7);
						}
						else {
							writeMessage(newfd, "RRQ EUSER\n", 10);
						}
					}
					else {
						writeMessage(newfd, "RRQ EFOP\n", 9);
						exit(0);
					}
				}
				else if(strcmp(arg1, "AUT") == 0) {

				}


				close(newfd);
				exit(0);
			}
			do {
				ret = close(newfd);
			} while(ret == -1 && errno == EINTR);

			if(ret == -1) {
				perror("close()");
				exit(1);
			}
		}
	}

	freeaddrinfo(asudpres);
	close(asudpfd);
	exit(0);
}

bool userIsLoggedIn(char* UID){
	char filename[32];

	sprintf(filename, "AS/USERS/%s/%s_login.txt", UID, UID);
	if(fopen(filename, "r") == NULL){
		return false;
	}
	fclose(filename);

	sprintf(filename, "AS/USERS/%s/%s_reg.txt", UID, UID);
	if(fopen(filename, "r") == NULL){
		return false;
	}
	fclose(filename);

	return true;
}

bool userExists(char* UID){
	char dirname[16];
	sprintf(dirname, "AS/USERS/%s", UID);

	d = opendir(dirname);

	if(!d) {
		return false;
	}
	return true;
}