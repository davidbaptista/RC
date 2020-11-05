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
#define AUX_SIZE 64

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

bool userIsLoggedIn(char* UID){
	FILE *fp;
	char filename[64];

	sprintf(filename, "AS/USERS/%s/%s_login.txt", UID, UID);
	if((fp = fopen(filename, "r")) == NULL){
		return false;
	}

	if(fclose(fp) != 0) {
		perror("fclose()");
		exit(1);
	}

	sprintf(filename, "AS/USERS/%s/%s_reg.txt", UID, UID);
	if((fp = fopen(filename, "r")) == NULL) {
		return false;
	}

	if(fclose(fp) != 0) { 
		perror("fclose()");
		exit(1);
	}

	return true;
}

bool userExists(char* UID){
	DIR *d;
	char dirname[64];
	sprintf(dirname, "AS/USERS/%s", UID);

	d = opendir(dirname);

	if(!d) {
		return false;
	}

	if(closedir(d) != 0) {
		perror("closedir()");
		exit(1);
	}
	return true;
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
	char dirname[64];
	char filename[128];
	char aux[AUX_SIZE];
	char buffer[BUFFER_SIZE];
	char arg1[16], arg2[16], arg3[16], arg4[16], arg5[16];
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
				printf(buffer);
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
								printf("RRG NOK\n");
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

								n = fwrite(arg3, 1, 8, fp);

								if(n != 8) {
									perror("fwrite()");

									if(fclose(fp) != 0) {
										perror("fclose()");
									}
									exit(1);
								}

								n = sendto(asudpfd, "RRG OK\n", 7, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

								if(n == -1) {
									perror("sendto()");
									exit(1);
								}

								if(verbose) {
									printf("RRG OK\n");
								}

								if(fclose(fp) != 0) {
									perror("fclose()");
									exit(1);
								}
							}
							//user already registrated
							else {
								n = fread(buffer, 1, BUFFER_SIZE, fp);
								buffer[n] = '\0';

								if(fclose(fp) != 0) {
									perror("fclose()");
									exit(1);
								}

								//verifies user's credentials
								if(strcmp(buffer, arg3) == 0) {
									n = sendto(asudpfd, "RRG OK\n", 7, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

									if(n == -1) {
										perror("sendto()");
										exit(1);
									}

									if(verbose) {
										printf("RRG OK\n");
									}
								}
								else {
									n = sendto(asudpfd, "RRG NOK\n", 8, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

									if(n == -1) {
										perror("sendto()");
										exit(1);
									}

									if(verbose) {
										printf("RRG NOK\n");
									}
								}
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

							if(fclose(fp) != 0) {
								perror("fclose()");
								exit(1);
							}

							if(closedir(d) != 0) {
								perror("closedir()");
								exit(1);
							}
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
						printf("RRG ERR\n");
					}
				}

			}
			else if(strcmp(arg1, "UNR") == 0) {
				n = sscanf(buffer, "%s %s %s", arg1, arg2, arg3);

				if(n == 3) {
					sprintf(dirname, "AS/USERS/%s", arg2);

					//user doesnt exist
					if(!userExists(arg2)) {
						sendto(asudpfd, "RUN NOK\n", 8, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);
						if(n < 0) {
							perror("sendto()");
							exit(1);
						}

						if(verbose) {
							printf("RUN NOK\n");
						}
					}
					//user exists
					else {
						sprintf(filename, "%s/%s_pass.txt", dirname, arg2);

						fp = fopen(filename, "r");

						if(fp == NULL) {
							perror("fopen()");
							exit(1);
						}

						n = fread(buffer, 1, BUFFER_SIZE, fp);
						buffer[n] = '\0';

						if(fclose(fp) != 0) {
							perror("fclose()");
							exit(1);
						}

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
									printf("RUN OK\n");
								}
							}
							else {
								sendto(asudpfd, "RUN NOK\n", 8, 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

								if(n < 0) {
									perror("sendto()");
									exit(1);
								}

								if(verbose) {
									printf("RUN NOK\n");
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
								printf("RUN NOK\n");
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
			else if(strcmp(arg1, "VLD") == 0) {
				n = sscanf(buffer, "VLD %s %s", arg1, arg2);

				if(n == 2 && strlen(arg1) == 5 && strlen(arg2) == 4 /* && userExists(arg1) && userIsLoggedIn(arg1)*/) {
					sprintf(filename, "AS/USERS/%s/%s_tid.txt", arg1, arg1);

					fp = fopen(filename, "r");

					if(fp == NULL) {
						sprintf(buffer, "CNF %s %s E\n", arg1, arg2);

						n = sendto(asudpfd, buffer, strlen(buffer), 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

						if(verbose) {
							printf(buffer);
						}

						if(n < 0) {
							perror("sendto()");
							exit(1);
						}

						continue;
					}

					n = fread(buffer, 1, 5, fp);

					buffer[n - 1] = '\0';	// apagar o espaco

					if(strcmp(buffer, arg2) == 0) {
						n = fread(aux, 1, AUX_SIZE, fp);
						aux[n] = '\0';

						if(fclose(fp) != 0) {
							perror("fclose()");
							exit(1);
						}

						sprintf(buffer, "CNF %s %s %s\n", arg1, arg2, aux);

						n = sendto(asudpfd, buffer, strlen(buffer), 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);

						// remover o ficheiro do tid
						if(remove(filename) != 0) {
							perror("remove()");
							exit(1);
						}

						if(aux[0] == 'X') {
							bool ok = true;

							sprintf(dirname, "AS/USERS/%s", arg1);
							d = opendir(dirname);

							if(d == NULL) {
								perror("opendir()");
								exit(1);
							}

							while((dir = readdir(d)) != NULL) {
								if(strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
									continue;
								}

								sprintf(buffer, "%s/%s", dirname, dir->d_name);
								if(remove(buffer) != 0) {
									ok = false;
								}
							}

							if(!ok) {
								perror("remove()");
								exit(1);
							} 
							else {
								if(rmdir(dirname) != 0) {
									perror("rmdir()");
									exit(1);
								}
							}
						}
					}
					else {
						sprintf(buffer, "CNF %s %s E\n", arg1, arg2);

						n = sendto(asudpfd, buffer, strlen(buffer), 0, (struct sockaddr*)&asudpaddr, asudpaddrlen);
					}

					if(n < 0) {
						perror("sendto()");
						exit(1);
					}

					if(verbose) {
						printf(buffer);
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
					printf("ERR\n");
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

				char RID[5];
				char UID[6];
				char Fop[2];
				char FName[25];
				int VC;
				int TID;
				int potentialVC;

				while(true) {
					n = readMessage(newfd, buffer);

					if(n < 0) {
						perror("read()");
						exit(1);
					}

					buffer[n] = '\0';

					sscanf(buffer, "%s ", arg1);
					
					if(strcmp(arg1, "LOG") == 0) {
						ret = sscanf(buffer, "LOG %s %s\n", arg2, arg3);

						if(verbose) {
							printf(buffer);
						}

						if(ret != 2 || strlen(arg2) != 5 || strlen(arg3) != 8) {
							writeMessage(newfd, "RLO ERR\n", 8);
							continue;
						}

						sprintf(dirname, "AS/USERS/%s", arg2);

						if(!userExists(arg2)) {
							writeMessage(newfd, "RLO ERR\n", 8);
							continue;
						}

						sprintf(filename, "%s/%s_reg.txt", dirname, arg2);

						fp = fopen(filename, "r");

						if(fp == NULL) {
							writeMessage(newfd, "RLO ERR\n", 8);
							continue;
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
							continue;
						}

						n = fread(buffer, 1, BUFFER_SIZE, fp);
						buffer[n] = '\0';

						if(fclose(fp) != 0) {
							perror("fclose()");
							exit(1);
						}

						if(strcmp(buffer, arg3) == 0) {
							sprintf(filename, "%s/%s_login.txt", dirname, arg2);
							strcpy(UID, arg2);

							if((fp = fopen(filename, "w")) == NULL) {
								perror("fopen()");
								exit(1);
							}

							if(fclose(fp) != 0) {
								perror("fclose()");
								exit(1);
							}

							writeMessage(newfd, "RLO OK\n", 7);
							printf("RLO OK\n");
						}
						else {
							writeMessage(newfd, "RLO NOK\n", 8);
							printf("RLO NOK\n");
						}
					}
					else if(strcmp(arg1, "REQ") == 0) {
						ret = sscanf(buffer, "REQ %s %s %s %s", arg1, arg2, arg3, arg4);

						if(verbose) {
							printf(buffer);
						}

						if(strlen(arg1) != 5 || strlen(arg2) != 4) {
							writeMessage(newfd, "RRQ ERR\n", 8);
							printf("RRQ ERR\n");
							continue;
						}

						if(!userExists(arg1)) {
							writeMessage(newfd, "RRQ EUSER\n", 10);
							printf("RRQ EUSER\n");
							continue;
						}

						if(userIsLoggedIn(arg1) != true || strcmp(UID, arg1) != 0) {
							writeMessage(newfd, "RRQ ELOG\n", 9);
							printf("RRQ ELOG\n");
							continue;
						}

						if(strcmp(arg3, "X") == 0 || strcmp(arg3, "L") == 0 || strcmp(arg3, "R") == 0 || strcmp(arg3, "U") == 0 || strcmp(arg3, "D") == 0) {
							bool hasFname = false;

							if((strcmp(arg3, "R") == 0 || strcmp(arg3, "U") == 0 || strcmp(arg3, "D") == 0) && ret == 4) {
								if(strlen(arg4) <= 24) {
									hasFname = true;
								}
								else {
									writeMessage(newfd, "RRQ ERR\n", 8);
									printf("RRQ ERR\n");
									continue;
								}
							}
							else if((strcmp(arg3, "L") == 0 || strcmp(arg3, "X") == 0) && ret == 3) {
								hasFname = false;
							}
							else {
								writeMessage(newfd, "RRQ ERR\n", 8);
								printf("RRQ ERR\n");
								continue;
							}

							sprintf(filename, "AS/USERS/%s/%s_reg.txt", arg1, arg1);
								
							if((fp = fopen(filename, "r")) == NULL){
								perror("fopen()");
								exit(1);
							}

							n = fread(buffer, 1, BUFFER_SIZE, fp);
							buffer[n] = '\0';

							char pdIP[32];
							char pdPort[8];
							sscanf(buffer, "%s %s", pdIP, pdPort);

							if((getaddrinfo(pdIP, pdPort, &pdhints, &pdres)) != 0) {
								perror("getaddrinfo()");
								exit(1);
							}

							VC = rand() % 10000;
							strcpy(RID, arg2);

							if(hasFname) {
								sprintf(buffer, "VLC %s %04d %s %s\n", arg1, VC, arg3, arg4);
								strcpy(FName, arg4);
							}
							else {
								sprintf(buffer, "VLC %s %04d %s\n", arg1, VC, arg3);
							}

							strcpy(Fop, arg3);

							n = sendto(pdfd, buffer, strlen(buffer), 0, pdres->ai_addr, pdres->ai_addrlen);
							if(n == -1) {
								exit(1);
							}

							n = recvfrom(pdfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&pdaddr, &pdaddrlen);
							buffer[n]= '\0';

							sscanf(buffer, "RVC %s %s", arg1, arg2);
							if(strcmp(arg2, "OK") == 0){
								writeMessage(newfd, "RRQ OK\n", 7);
								printf("RRQ OK\n");
							}
							else {
								writeMessage(newfd, "RRQ EUSER\n", 10);
								printf("RRQ EUSER\n");
							}
						}
						else {
							writeMessage(newfd, "RRQ EFOP\n", 9);
							printf("RRQ EFOP\n");
						}
					}
					else if(strcmp(arg1, "AUT") == 0) {
						ret = sscanf(buffer, "AUT %s %s %s\n", arg1, arg2, arg3);

						if(verbose) {
							printf(buffer);
						}

						potentialVC = strtol(arg3, NULL, 10);

						if(ret == 3 && strlen(arg1) == 5 && strlen(arg2) == 4) {
							if(userExists(arg1) && userIsLoggedIn(arg1) && strcmp(arg1, UID) == 0 && strcmp(arg2, RID) == 0 && potentialVC == VC) {
								TID = rand() % 10000;

								sprintf(filename, "AS/USERS/%s/%s_tid.txt", UID, UID);

								fp = fopen(filename, "w");

								if(fp == NULL) {
									perror("fopen()");
									exit(1);
								}

								if(strcmp(Fop, "L") == 0 || strcmp(Fop, "X") == 0) {
									sprintf(buffer, "%04d %s", TID, Fop);
								}
								else {
									sprintf(buffer, "%04d %s %s", TID, Fop, FName);
								}

								fwrite(buffer, 1, strlen(buffer), fp);

								if(fclose(fp) != 0) {
									perror("fclose()");
									exit(1);
								}

								sprintf(buffer, "RAU %04d\n", TID);

								writeMessage(newfd, buffer, strlen(buffer));
							}
							else {
								writeMessage(newfd, "RAU 0\n", 6);
							}
						}
						else {
							writeMessage(newfd, "RAU 0\n", 6);
						}
					}
					else if(strcmp(arg1, "EXIT") == 0) {
						sscanf(buffer, "EXIT %s", arg2);
						writeMessage(newfd, "EXIT\n", 5);
						sprintf(filename, "AS/USERS/%s/%s_login.txt", arg2, arg2);
						remove(filename);
						printf("EXIT\n");

						break;
					}
					else { // TODO: problemas aqui
						writeMessage(newfd, "ERR\n", 4);
					//	printf("ERR\n");
					}
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
