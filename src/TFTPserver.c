/*

** Fichero: TFTPserver.c
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "utils.h"
#include "msgUtils.h"

#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define MAXHOST 128

bool end;
bool timeOutPassed;
bool ErrorisReadMode;
int nRetries;
extern int errno;

char SIGALRMhostName[80];
char SIGALRMhostIp[80];
char SIGALRMfileName[80];
int SIGALRMport;
int SIGALRMs;
FILE * SIGALRMf;

void SIGALRMHandler(int ss);
void SIGTERMHandler(int ss);

void TFTPserverReadMode(ProtocolMode mode,int s, char * hostName, char * hostIp, int port, char * fileName, struct sockaddr_in cientaddr_in);
void TFTPserverWriteMode(ProtocolMode mode,int s, char * hostName, char * hostIp, int port, char * fileName, struct sockaddr_in cientaddr_in);

const char * getDateAndTime();
void logError(char * hostName, char * hostIp, char * fileName, char * protocol, int port, int errorCode, char * errormsg);
void logConnection(char * hostName, char * hostIp, char * fileName, char * protocol, int port, int block, int mode);

void redefineSignal(int signal, void(*function)(int)){
  struct sigaction ss;
  memset(&ss,0,sizeof(ss));

  ss.sa_handler=function;
  ss.sa_flags=0;

  if(-1 == sigfillset(&ss.sa_mask)){
    fprintf(stderr,"\nSignal error");
    exit(EXIT_FAILURE);
  }
    
  if(-1 == sigaction(signal,&ss,NULL)){
    fprintf(stderr,"\nSignal error");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char * argv[]){
	sigset_t signalSet;
	rwMsg requestmsg;
	struct linger lngr;
    
	int port,msgSize;
	char hostIp[50];
	char hostName[50];

	//initialize end to false
	end = FALSE;

	//Block all signals and redefine sigterm and sigalarm
	redefineSignal(SIGCHLD,	SIG_IGN);
	redefineSignal(SIGTERM,SIGTERMHandler);
	redefineSignal(SIGALRM,SIGALRMHandler);

	if(-1 == sigfillset(&signalSet)){
		fprintf(stderr,"\nSignal error");
		exit(EXIT_FAILURE);
	}
	if(-1 == sigdelset(&signalSet,SIGTERM)){
		fprintf(stderr,"\nSignal error");
		exit(EXIT_FAILURE);
	}
	if(-1 == sigdelset(&signalSet,SIGALRM)){
		fprintf(stderr,"\nSignal error");
		exit(EXIT_FAILURE);
	}
	if(-1 == sigdelset(&signalSet,SIGCHLD)){
		fprintf(stderr,"\nSignal error");
		exit(EXIT_FAILURE);
	}
	if(-1 ==sigprocmask(SIG_SETMASK, &signalSet, NULL)){
		fprintf(stderr,"\nSignal error");
		exit(EXIT_FAILURE);
	}

	//initialize sigalarm error parameters
	memset(SIGALRMhostName,0,80);
	memset(SIGALRMhostIp,0,80);
	memset(SIGALRMfileName,0,80);
	SIGALRMport = -1;
	
	//Server initialization
	int s_TCP, s_UDP, ls_TCP;
	struct sockaddr_in myaddr_in;
	struct sockaddr_in clientaddr_in;
	int addrlen;
	fd_set readmask;
	int numfds,s_mayor;
	char buffer[TAM_BUFFER];

	/* Create the listen socket. */
	ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

	addrlen = sizeof(struct sockaddr_in);

	/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
	/* The server should listen on the wildcard address,
	* rather than its own internet address.  This is
	* generally good practice for servers, because on
	* systems which are connected to more than one
	* network at once will be able to have one server
	* listening on all networks at once.  Even when the
	* host is connected to only one network, this is good
	* practice, because it makes the server program more
	* portable. */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PORT);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
        fprintf(stderr,": unable to bind address TCP\n");
        close(ls_TCP);
		exit(EXIT_FAILURE);
	}
	/* Initiate the listen on the socket so remote users
	* can connect.  The listen backlog is set to 5, which
	* is the largest currently supported. */
	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
        fprintf(stderr,": unable to listen on socket\n");
        close(ls_TCP);
		exit(EXIT_FAILURE);
	}

	/* Create the socket UDP. */
	s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1) {
        fprintf(stderr,": unable to create socket UDP\n");
        close(ls_TCP);
		exit(EXIT_FAILURE);
	}
	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
        fprintf(stderr,": unable to bind address UDP\n");
        close(s_UDP);
        close(ls_TCP);
		exit(EXIT_FAILURE);
	}

  	setpgrp();

	pid_t daemonPid;
	if(-1 == (daemonPid = fork())){
		fprintf(stderr,"\nUnable to fork daemon");
        close(s_UDP);
        close(ls_TCP);
		exit(EXIT_FAILURE);
	}else if(0 != daemonPid){
		/*If is father it exits*/
		exit(EXIT_SUCCESS);
	}

	/* The child process (daemon) comes here. */
	fclose(stdin);
	fclose(stderr);

	while (!end) {
		/* Meter en el conjunto de sockets los sockets UDP y TCP */
		FD_ZERO(&readmask);
		FD_SET(ls_TCP, &readmask);
		FD_SET(s_UDP, &readmask);
		/*Seleccionar el descriptor del socket que ha cambiado. Deja una marca en
        	el conjunto de sockets (readmask) */
		s_mayor = (ls_TCP > s_UDP) ? ls_TCP : s_UDP;
		numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL);
		if(EINTR ==errno){
	        close(s_UDP);
	        close(ls_TCP);
			exit(EXIT_FAILURE);
		}


		/* Comprobamos si el socket seleccionado es el socket TCP */
		if (FD_ISSET(ls_TCP, &readmask)) {

			if(-1 ==(s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen))){
                logError("UNKNOWN", "UNKNOWN","UNKNOWN", "TCP", -1, -1,"\nUnable to accept tcp socket from parent");
		        close(s_UDP);
		        close(ls_TCP);
				exit(EXIT_FAILURE);
			}

			pid_t serverInstancePid;
			if(-1 == (serverInstancePid = fork())){
                logError("UNKNOWN", "UNKNOWN","UNKNOWN", "TCP", -1, -1,"\nUnable to fork tcp server instance");
		        close(s_UDP);
		        close(s_TCP);
		        close(ls_TCP);
				exit(EXIT_FAILURE);
			}

			if(0 == serverInstancePid){
                close(ls_TCP); 
                /* Close the listen socket inherited from the daemon. */
 				//obtain the host information to start communicacion with remote host
                if( getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),hostName,sizeof(hostName),NULL,0,0) ){
                    if(NULL == inet_ntop(AF_INET,&clientaddr_in.sin_addr,hostName,sizeof(hostName))){
                        logError("UNKNOWN", "UNKNOWN","UNKNOWN", "TCP", -1, -1,"\nError inet_ntop");
				        close(s_TCP);
                        exit(EXIT_FAILURE);
                    }
                }
                port = ntohs(clientaddr_in.sin_port);
                strcpy(hostIp, inet_ntoa(clientaddr_in.sin_addr));

                //set opMode
                lngr.l_onoff = 1;
                lngr.l_linger= 1;
                if(-1 == setsockopt(s_TCP,SOL_SOCKET,SO_LINGER,&lngr,sizeof(lngr))){
                    logError(hostName, hostIp,"UNKNOWN", "TCP", port,  -1,"\nError on making setsockopt");
				    close(s_TCP);
                    exit(EXIT_FAILURE);
                }

                //read client request to know if is required read or write
				memset(buffer,0,TAM_BUFFER);
                msgSize = recv(s_TCP,buffer,TAM_BUFFER,0);
                if(msgSize <= 0){
                    msgSize = fillBufferWithErrMsg(UNKNOWN,"error on receiving client request", buffer);
                    if(send(s_TCP, buffer, msgSize, 0) != msgSize){
                        logError(hostName, hostIp,"UNKNOWN", "TCP", port, -1,"\nError on sending error msg");
                    }
                    logError(hostName, hostIp,"UNKNOWN", "TCP", port, -1, "error on receiving client request");
                    close(s_TCP);
                    exit(EXIT_FAILURE);
                }

                requestmsg = fillReadMsgWithBuffer(buffer);
                //if the msg isnt a request send error
                if(READ_TYPE != requestmsg.header && WRITE_TYPE != requestmsg.header){
                    msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,"error on client request header: isn't read or write", buffer);
                    if(send(s_TCP, buffer, msgSize, 0) != msgSize){
                        logError("UNKNOWN", "UNKNOWN","UNKNOWN", "TCP", -1, -1,"\nError sending error response message");
                    }
                    logError(hostName, hostIp,requestmsg.fileName, "TCP", port, ILLEGAL_OPERATION, "error on client request header: isn't read or write");
                    close(s_TCP);
                    exit(EXIT_FAILURE);
                }

                if(requestmsg.header == READ_TYPE)
                    TFTPserverReadMode(TCP_MODE,s_TCP,hostName,hostIp,port,requestmsg.fileName,clientaddr_in);
                else
                    TFTPserverWriteMode(TCP_MODE,s_TCP,hostName,hostIp,port,requestmsg.fileName,clientaddr_in);
                exit(EXIT_SUCCESS);
                  		
			}else{
                close(s_TCP);
            }

        }


		/* Comprobamos si el socket seleccionado es el socket UDP */
		if (FD_ISSET(s_UDP, &readmask)) {
			nRetries = 0;
			do{
				memset(buffer,0,TAM_BUFFER);
				timeOutPassed = FALSE;
				alarm(TIMEOUT);
				if(-1 == (msgSize = recvfrom(s_UDP, buffer, TAM_BUFFER, 0, (struct sockaddr*)&clientaddr_in, &addrlen))){
	                logError("UNKNOWN", "UNKNOWN","UNKNOWN", "UDP", -1, -1,"\nRecvfrom error");
			        close(s_UDP);
			        close(ls_TCP);
					exit(EXIT_FAILURE);
				}
			}while(timeOutPassed);
			alarm(0);

			pid_t serverInstancePid;
			if((serverInstancePid = fork()) == -1){
                logError("UNKNOWN", "UNKNOWN","UNKNOWN", "UDP", -1, -1,"\nUnable to fork tcp server instance");
		        close(s_UDP);
		        close(ls_TCP);
				exit(EXIT_FAILURE);
			}
			if(0 == serverInstancePid){
				//create new port (efimero) (soket + bind)
				int s;
				struct sockaddr_in myaddrin;

				myaddrin.sin_family = AF_INET;
				myaddrin.sin_port = 0;
				myaddrin.sin_addr.s_addr = INADDR_ANY;

				if(getnameinfo((struct sockaddr *)&clientaddr_in ,sizeof(clientaddr_in), hostName,sizeof(hostName),NULL,0,0)){
				   	if (NULL == inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostName, sizeof(hostName))){
                        logError("UNKNOWN", "UNKNOWN","UNKNOWN", "UDP", -1, -1,"Error inet_ntop");
				        close(s_TCP);
                        exit(EXIT_FAILURE);
				   	}
				}
				port = ntohs(clientaddr_in.sin_port);
                strcpy(hostIp, inet_ntoa(clientaddr_in.sin_addr));

				/* Create the socket UDP. */
				s = socket (AF_INET, SOCK_DGRAM, 0);
				if (s == -1) {
                    logError(hostName, hostIp,"UNKNOWN", "UDP", port, -1," unable to create socket UDP\n");
					exit(EXIT_FAILURE);
			    }
				/* Bind the server's address to the socket. */
				if (bind(s, (struct sockaddr *) &myaddrin, sizeof(struct sockaddr_in)) == -1) {
                    logError(hostName, hostIp,"UNKNOWN", "UDP", port, -1," unable to bind address UDP\n");
			        close(s);
					exit(EXIT_FAILURE);
			    }
			      	
			    if(getsockname(s, (struct sockaddr *)&myaddrin, &addrlen) == -1){
                    logError(hostName, hostIp,"UNKNOWN", "UDP", port, -1,"\nUnable to read socket address");
			        close(s);
					exit(EXIT_FAILURE);
				}

				//read request
				requestmsg = fillReadMsgWithBuffer(buffer);
				//if the msg isnt a request send error 
				if(READ_TYPE != requestmsg.header && WRITE_TYPE != requestmsg.header){
 					msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,"error on client request header: isn't read or write", buffer);
                    if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                        logError(hostName, hostIp,"UNKNOWN", "UDP", port, -1, "\nUnknown error produced when sending error msg");
                    }
					logError(hostName, hostIp,"UNKNOWN", "UDP", port, ILLEGAL_OPERATION, "error on client request header: isn't read or write");
			        close(s);
					exit(EXIT_FAILURE);
				}

				if(requestmsg.header == READ_TYPE)
					TFTPserverReadMode(UDP_MODE,s,hostName,hostIp,port,requestmsg.fileName,clientaddr_in);
				else
					TFTPserverWriteMode(UDP_MODE,s,hostName,hostIp,port,requestmsg.fileName,clientaddr_in);

				exit(EXIT_SUCCESS);
			}else{
				continue;
			}
		
		}

		//initialize sigalarm error parameters
		memset(SIGALRMhostName,0,80);
		memset(SIGALRMhostIp,0,80);
		memset(SIGALRMfileName,0,80);
		SIGALRMport = -1;
	}

	close(s_UDP);
	close(ls_TCP);
	exit(EXIT_SUCCESS);
}

void SIGTERMHandler(int ss){
    end = TRUE;
}

void SIGALRMHandler(int ss){

	if(nRetries<RETRIES){
		nRetries++;
		timeOutPassed = TRUE;
	}else{
		logError(SIGALRMhostName, SIGALRMhostIp,SIGALRMfileName, "UDP", SIGALRMport, UNKNOWN, "Timeout passed");
		close(SIGALRMs);
		if(NULL != SIGALRMf)
			fclose(SIGALRMf);
		exit(EXIT_FAILURE);
	}
}

void TFTPserverReadMode(ProtocolMode mode,int s, char * hostName, char * hostIp, int port, char * fileName, struct sockaddr_in clientaddr_in){
	bool endSesion = FALSE;

	rwMsg requestMsg;
	dataMsg datamsg;
	ackMsg ackmsg;
	errMsg errmsg;
	headers msgType;

	FILE * f = NULL;
	int blockNumber;
	int msgSize, readSize;
	char dataBuffer[MSG_DATA_SIZE];
	char buffer[TAM_BUFFER];	

	int addrlen = sizeof(struct sockaddr_in);

	logConnection(hostName, hostIp,fileName, MODE_STR(mode), port, 0, LOG_START_WRITE);

	if(mode == UDP_MODE){
		strcpy(SIGALRMhostName, hostName);
		strcpy(SIGALRMhostIp, hostIp);
		strcpy(SIGALRMfileName, fileName);
		SIGALRMport = port;
		SIGALRMf = NULL;
		SIGALRMs = s;
	}

	ErrorisReadMode = TRUE;

	char finalPath[30];
	sprintf(finalPath,"%s/%s",SERVER_FILES_FOLDER,fileName);

	//check if file exists
	bool fileExists = (access(finalPath, F_OK ) != -1) ? TRUE : FALSE;	

	//send error if file not found
	if(!fileExists){
		msgSize = fillBufferWithErrMsg(FILE_NOT_FOUND,"server coundn't found the requested file", buffer);
		if(mode==TCP_MODE){
		    if(send(s, buffer, msgSize, 0) != msgSize){
		        logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "Error sending error response message");
		    }
		}else{
		    if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
		        logError(hostName, hostIp,fileName, MODE_STR(mode), port,  -1, "Error sending error response message");
		    }
		}
		logError(hostName, hostIp,fileName, MODE_STR(mode), port, FILE_NOT_FOUND, "server coundn't found the requested file");
		close(s);
		exit(EXIT_FAILURE);
	}

	//open the request file
	if(NULL == (f = fopen(finalPath,"rb"))){
		//if error send error msg 
		msgSize = fillBufferWithErrMsg(UNKNOWN, "server couldn't open file", buffer);
        if(mode==TCP_MODE){
            if(send(s, buffer, msgSize, 0) != msgSize){
                logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
            }
        }else{
            if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
            }
        }
		logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "server couldn't open file");
		close(s);
		exit(EXIT_FAILURE);
	}

	if(mode == UDP_MODE){
		SIGALRMf = f;
	}

	blockNumber = 1;
	endSesion = FALSE;
	while(!endSesion){
		//read data block from file
		memset(dataBuffer,0,MSG_DATA_SIZE);
		readSize = fread(dataBuffer, 1, MSG_DATA_SIZE, f);
		if(-1 == readSize){
			msgSize = fillBufferWithErrMsg(UNKNOWN,"Error reading the file",buffer);
            if(mode==TCP_MODE){
                if(send(s, buffer, msgSize, 0) != msgSize){
                    logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
                }
            }else{
                if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                    logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
                }
            }
			logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Error reading the file");
			close(s);			
			fclose(f);
			exit(EXIT_FAILURE);
		}

		//send the readed data block and recive ack
		if(mode == TCP_MODE){
			//send block
			msgSize = fillBufferWithDataMsg(blockNumber,dataBuffer,readSize,buffer);
			if(msgSize != send(s, buffer, msgSize, 0)){
                msgSize = fillBufferWithErrMsg(UNKNOWN, "Unable to send data block", buffer);
                if(send(s, buffer, msgSize, 0) != msgSize){
                    logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
                }
				logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Unable to send data block");
				close(s);			
				fclose(f);
				exit(EXIT_FAILURE);
			}
			//wait for ack
			memset(buffer,0,TAM_BUFFER);
			msgSize = recv(s, buffer, TAM_BUFFER, 0);
			if(msgSize < TAM_BUFFER && msgSize>0) buffer[msgSize] = '\0';
		}else{
			nRetries = 0;
			do{
				//send block
				msgSize = fillBufferWithDataMsg(blockNumber,dataBuffer,readSize,buffer);
				if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in))){
	                msgSize = fillBufferWithErrMsg(UNKNOWN, "Unable to send data block", buffer);
	                if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
	                    logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
	                }
					logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "Unable to send data block");
					close(s);			
					fclose(f);
					exit(EXIT_FAILURE);
				}
				//wait for ack
				memset(buffer,0,TAM_BUFFER);
				timeOutPassed = FALSE; 
				alarm(TIMEOUT);
				msgSize = recvfrom(s, buffer, TAM_BUFFER, 0,(struct sockaddr*)&clientaddr_in,&addrlen);
				if(msgSize < TAM_BUFFER && msgSize>0) buffer[msgSize] = '\0';
			}while(timeOutPassed);
			alarm(0);
		}
	
		if(msgSize <= 0){
           	    msgSize = fillBufferWithErrMsg(UNKNOWN, "Unable to recive ack", buffer);
		    if(mode==TCP_MODE){
		        if(send(s, buffer, msgSize, 0) != msgSize){
		            logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
		        }
		    }else{
		        if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
		            logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
		        }
		    }
			logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "Unable to recive ack");
			close(s);			
			fclose(f);
			exit(EXIT_FAILURE);	
		}

		switch(getMessageTypeWithBuffer(buffer)){
			case ACK_TYPE: 
				ackmsg = fillAckWithBuffer(buffer);
				//check if ack its correct; if not an error is send
				if( blockNumber != ackmsg.blockNumber ){
                    msgSize = fillBufferWithErrMsg(UNKNOWN, "Recived ACK for block which doesn't matches with current", buffer);
                    if(mode==TCP_MODE){
                        if(send(s, buffer, msgSize, 0) != msgSize){
                            logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
                        }
                    }else{
                        if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                            logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
                        }
                    }
					logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Recived ACK for block which doesn't matches with current");
					close(s);			
					fclose(f);
					exit(EXIT_FAILURE);
				}
				blockNumber += 1;
			break;
			default:	
                msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION, "Unrecognized operation in current context", buffer);
                if(mode==TCP_MODE){
                    if(send(s, buffer, msgSize, 0) != msgSize){
                        logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
                    }
                }else{
                    if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                        logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
                    }
                }
				logError(hostName, hostIp,fileName, MODE_STR(mode), port, -ILLEGAL_OPERATION, "Unrecognized operation in current context");
				close(s);			
				fclose(f);
				exit(EXIT_FAILURE);
			break; 
		}

		//check if the file has ended
		if(feof(f))
			endSesion = TRUE;
	}

	close(s);
	fclose(f);
	logConnection(hostName, hostIp,fileName, MODE_STR(mode), port, blockNumber-2, LOG_END_READ);
}

void TFTPserverWriteMode(ProtocolMode mode,int s, char * hostName, char * hostIp, int port, char * fileName, struct sockaddr_in clientaddr_in){

	bool endSesion = FALSE;

	rwMsg requestMsg;
	dataMsg datamsg;
	ackMsg ackmsg;
	errMsg errmsg;

	FILE * f = NULL;
	int blockNumber;
	int msgSize, writeResult;
	char dataBuffer[MSG_DATA_SIZE];
	char buffer[TAM_BUFFER];	

	int addrlen = sizeof(struct sockaddr_in);

	if(mode == UDP_MODE){
		strcpy(SIGALRMhostName, hostName);
		strcpy(SIGALRMhostIp, hostIp);
		strcpy(SIGALRMfileName, fileName);
		SIGALRMport = port;
		SIGALRMf = NULL;
		SIGALRMs = s;
	}

	ErrorisReadMode = FALSE;

	logConnection(hostName, hostIp,fileName, MODE_STR(mode), port, 0, LOG_START_READ);

	char finalPath[30];
	sprintf(finalPath,"%s/%s",SERVER_FILES_FOLDER,fileName);

	//check if file exists
	bool fileExists = (access( finalPath, F_OK ) != -1) ? TRUE : FALSE;

	//send error if file alredy exsits
	if(fileExists){
		msgSize = fillBufferWithErrMsg(FILE_ALREADY_EXISTS,"The requested file already exists", buffer);
		if(mode==TCP_MODE){
		    if(send(s, buffer, msgSize, 0) != msgSize){
		        logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error message");
		    }
		}else{
		    if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
		        logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error message");
		    }
		}
		logError(hostName, hostIp,fileName, MODE_STR(mode), port, FILE_ALREADY_EXISTS, "The requested file already exists");
		close(s);
		exit(EXIT_FAILURE);
	}

	//open the file to write
	if(NULL == (f = fopen(finalPath,"wb"))){
		//if error send error msg 
        msgSize = fillBufferWithErrMsg(UNKNOWN, "client couldn't open file", buffer);
        if(mode==TCP_MODE){
            if(send(s, buffer, msgSize, 0) != msgSize){
                logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
            }
        }else{
            if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
            }
        }
		logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "client couldn't open file");
		close(s);
		exit(EXIT_FAILURE);
	}
	
	if(mode == UDP_MODE){
		SIGALRMf = f;
	}

	blockNumber = 0;
	//send ack and wait for data blcok 
	if(mode == TCP_MODE){
		//send ack 
		msgSize = fillBufferWithAckMsg(blockNumber, buffer);
		if(msgSize != send(s, buffer, msgSize, 0)){
            msgSize = fillBufferWithErrMsg(UNKNOWN, "Unable to send ack", buffer);
            if(send(s, buffer, msgSize, 0) != msgSize){
                logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
            }
			logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Unable to send ack");
			close(s);			
			fclose(f);
			exit(EXIT_FAILURE);
		}
		//wait for data block 
		memset(buffer,0,TAM_BUFFER);
		msgSize = recv(s, buffer, TAM_BUFFER, 0);
		if(msgSize < TAM_BUFFER && msgSize>0) buffer[msgSize] = '\0';
	}else{
		nRetries = 0;
		do{
			//send ack 
			msgSize = fillBufferWithAckMsg(blockNumber, buffer);
			if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in))){
	            msgSize = fillBufferWithErrMsg(UNKNOWN, "Unable to send ack", buffer);
	            if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
	                logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
	            }
				logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Unable to send ack");
				close(s);			
				fclose(f);
				exit(EXIT_FAILURE);
			}
			//wait for data block 
			memset(buffer,0,TAM_BUFFER);
			timeOutPassed = FALSE;
			alarm(TIMEOUT);
			msgSize = recvfrom(s, buffer, TAM_BUFFER, 0,(struct sockaddr*)&clientaddr_in,&addrlen);
			if(msgSize < TAM_BUFFER && msgSize>0) buffer[msgSize] = '\0';
		}while(timeOutPassed);
		alarm(0);
	}
	if(msgSize <= 0){
        msgSize = fillBufferWithErrMsg(UNKNOWN, "Unable to recive data block", buffer);
        if(mode==TCP_MODE){
            if(send(s, buffer, msgSize, 0) != msgSize){
                logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
            }
        }else{
            if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
            }
        }
		logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Unable to recive data block");
		close(s);			
		fclose(f);
		exit(EXIT_FAILURE);	
	}

	blockNumber = 1;
	endSesion = FALSE;
	while(!endSesion){
		//act according to type
		switch(getMessageTypeWithBuffer(buffer)){
			case DATA_TYPE:
				datamsg = fillDataWithBuffer(msgSize,buffer);
				//check if block number is the correct one
				if(datamsg.blockNumber != blockNumber ){
                    msgSize = fillBufferWithErrMsg(UNKNOWN, "Recived data block which doesnt matches with current", buffer);
                    if(mode==TCP_MODE){
                        if(send(s, buffer, msgSize, 0) != msgSize){
                            logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
                        }
                    }else{
                        if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                            logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
                        }
                    }
					logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Recived data block which doesnt matches with current");
					close(s);			
					fclose(f);
					exit(EXIT_FAILURE);
				}
				//write the recived data
				writeResult = fwrite(datamsg.data,1,DATA_SIZE(msgSize),f);
				if(-1 == writeResult){
                    msgSize = fillBufferWithErrMsg(UNKNOWN, "Unable to write the file", buffer);
                    if(mode==TCP_MODE){
                        if(send(s, buffer, msgSize, 0) != msgSize){
                            logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
                        }
                    }else{
                        if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                            logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
                        }
                    }
					logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Unable to write the file");
					close(s);			
					fclose(f);
					exit(EXIT_FAILURE);
				}
				//check if the block is the last -- size less than 512 bytes
				if(DATA_SIZE(msgSize) < MSG_DATA_SIZE)
					endSesion = TRUE;
			break;
			default:	
                msgSize = fillBufferWithErrMsg(UNKNOWN, "Unrecognized operation in current context", buffer);
                if(mode==TCP_MODE){
                    if(send(s, buffer, msgSize, 0) != msgSize){
                        logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
                    }
                }else{
                    if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                        logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
                    }
                }
				logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Unrecognized operation in current context");
				close(s);			
				fclose(f);
				exit(EXIT_FAILURE);
			break; 
		}

		if(!endSesion){
			//send ack and recive data block
			msgSize = fillBufferWithAckMsg(blockNumber, buffer);
			if(mode == TCP_MODE){
				//send ack
				if(msgSize != send(s, buffer, msgSize, 0)){
	                msgSize = fillBufferWithErrMsg(UNKNOWN, "Unable to send ack", buffer);
	                if(send(s, buffer, msgSize, 0) != msgSize){
	                    logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
	                }
					logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Unable to send ack");
					close(s);			
					fclose(f);
					exit(EXIT_FAILURE);
				}
				//recive next data block
				memset(buffer,0,TAM_BUFFER);
				msgSize = recv(s, buffer, TAM_BUFFER, 0);
			  	if(msgSize < TAM_BUFFER && msgSize>0) buffer[msgSize] = '\0';
			}else{
				nRetries = 0;
				do{
					//send ack
					if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in))){
		                msgSize = fillBufferWithErrMsg(UNKNOWN, "Unable to send ack", buffer);
		                if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
		                    logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
		                }
						logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Unable to send ack");
						close(s);			
						fclose(f);
						exit(EXIT_FAILURE);
					}
					//recive next data block
					memset(buffer,0,TAM_BUFFER);
					timeOutPassed = FALSE;
					alarm(TIMEOUT);
					msgSize = recvfrom(s, buffer, TAM_BUFFER, 0,(struct sockaddr*)&clientaddr_in,&addrlen);
					if(msgSize < TAM_BUFFER) buffer[msgSize] = '\0';
				}while(timeOutPassed);
			}
			if(msgSize <= 0){
	            msgSize = fillBufferWithErrMsg(UNKNOWN, "Unable to recive data block", buffer);
	            if(mode==TCP_MODE){
	                if(send(s, buffer, msgSize, 0) != msgSize){
	                    logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
	                }
	            }else{
	                if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
	                    logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
	                }
	            }
				logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Unable to recive data block");
				close(s);			
				fclose(f);
				exit(EXIT_FAILURE);	
			}
        	//increment block number 
        	blockNumber += 1;
		}
				
	}

	//send last ack
	msgSize = fillBufferWithAckMsg(blockNumber, buffer);
	if(mode == TCP_MODE){
		if(msgSize != send(s, buffer, msgSize, 0)){
            msgSize = fillBufferWithErrMsg(UNKNOWN, "Unable to send ack", buffer);
            if(send(s, buffer, msgSize, 0) != msgSize){
                logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
            }
			logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Unable to send ack");
			close(s);			
			fclose(f);
			exit(EXIT_FAILURE);
		}
	}else{
		if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in))){
            msgSize = fillBufferWithErrMsg(UNKNOWN, "Unable to send ack", buffer);
            if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1, "\nError sending error response message");
            }
			logError(hostName, hostIp,fileName, MODE_STR(mode), port, -1 , "Unable to send ack");
			close(s);			
			fclose(f);
			exit(EXIT_FAILURE);
		}
	}


	close(s);
	fclose(f);
	logConnection(hostName, hostIp,fileName, MODE_STR(mode), port, blockNumber, LOG_END_WRITE);
}




void logConnection(char * hostName, char * hostIp, char * fileName, char * protocol, int port, int block, int mode){
	char toLog[LOG_MESSAGE_SIZE];

	FILE*logFile = fopen(SERVER_LOG,"a+"); 

	switch(mode){ 
		case LOG_START_READ:  
			sprintf(toLog,"\n[%s][HOST: %s][IP: %s][PROTOCOL: %s][PORT: %d][FILE: %s][STARTED READ]", getDateAndTime(), hostName, hostIp, protocol, port, fileName); 
		break;
		case LOG_START_WRITE: 
			sprintf(toLog,"\n[%s][HOST: %s][IP: %s][PROTOCOL: %s][PORT: %d][FILE: %s][STARTED WRITE]", getDateAndTime(), hostName, hostIp, protocol, port, fileName); 
		break;
		case LOG_END_READ:  
			sprintf(toLog,"\n[%s][HOST: %s][IP: %s][PROTOCOL: %s][PORT: %d][FILE: %s][BLOCKS: %d][READ SUCCED]", getDateAndTime(), hostName, hostIp, protocol, port, fileName,block); 
		break;
		case LOG_END_WRITE: 
			sprintf(toLog,"\n[%s][HOST: %s][IP: %s][PROTOCOL: %s][PORT: %d][FILE: %s][BLOCKS:%d][WRITE SUCCED]", getDateAndTime(), hostName, hostIp, protocol, port, fileName,block); 
		break;
	}

	fprintf(logFile,"%s",toLog);
	fclose(logFile);
}

void logError(char * hostName, char * hostIp, char * fileName, char * protocol, int port, int errorCode, char * errormsg){
	char error[30];
	char toLog[LOG_MESSAGE_SIZE];

	FILE*logFile = fopen(SERVER_LOG,"a+");    

	switch (errorCode) {
		case 0: strcpy(error,"UNKNOWN"); break;
		case 1: strcpy(error,"FILE_NOT_FOUND"); break;
		case 3: strcpy(error,"DISK_FULL"); break;
		case 4: strcpy(error,"ILEGAL_OPERATION"); break;
		case 6: strcpy(error,"FILE_ALREADY_EXISTS"); break;
		case -1: strcpy(error,"INTERNAL_SERVER_ERRROR"); break;
	}
	sprintf(toLog,"\n[%s][HOST: %s][IP: %s][PROTOCOL: %s][MODE: %s][PORT: %d][FILE: %s][ERROR : %s][MSG: %s]", getDateAndTime(), hostName, hostIp, protocol,(ErrorisReadMode?("READ"):("WRITE")), port, fileName,error, errormsg);

	fprintf(logFile, "%s",toLog);
	fclose(logFile);
}

const char * getDateAndTime(){
	static char timeString[TIME_STRING_SIZE];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(timeString,"%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	return timeString;
}
