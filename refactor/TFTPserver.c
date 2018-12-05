/*

** Fichero: TFTPserver.c
** Autores:s
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
extern int errno;

char SIGALRMHostName[30];
char SIGALRMHostIp[30];
char SIGALRMFileName[30];
int SIGALRMPort;
void SIGALRMHandler(int ss);
void SIGTERMHandler(int ss);

void TFTPserverReadMode(ProtocolMode mode,int s, char * hostName, char * hostIp, int port, char * fileName, struct sockaddr_in cientaddr_in);
void TFTPserverWriteMode(ProtocolMode mode,int s, char * hostName, char * hostIp, int port, char * fileName, struct sockaddr_in cientaddr_in);

const char * getDateAndTime();
void logError(char * hostName, char * hostIp, char * fileName, char * protocol, int port, int errorCode, char * errormsg);
void logConnection(char * hostName, char * hostIp, char * fileName, char * protocol, int port, int block, int mode);


#define EXIT_ON_WRONG_VALUE(wrongValue, errorMsg, returnValue)                          \
do{                                              		    	                              \
    if((returnValue) == (wrongValue)){		    	                                        \
        char errorTag[200];                                                             \
        sprintf(errorTag, "\n[%s:%d:%s]%s", __FILE__, __LINE__, __FUNCTION__,errorMsg); \
        logError("", "", "", "", -3, 1, "asdf");		    	                                            \
        raise(SIGTERM);		    	                                                        \
        exit(0);                                                                        \
    }		    	                                                                          \
}while(0)


void logIssue(char * asdf){

}
void redefineSignal(int signal, void(*function)(int)){
  struct sigaction ss;
  memset(&ss,0,sizeof(ss));

  ss.sa_handler=function;
  ss.sa_flags=0;

  EXIT_ON_WRONG_VALUE(-1,"signal error",sigfillset(&ss.sa_mask));
  EXIT_ON_WRONG_VALUE(-1,"signal error",sigaction(signal,&ss,NULL));
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
	if(-1 ==sigprocmask(SIG_SETMASK, &signalSet, NULL)){
		fprintf(stderr,"\nSignal error");
		exit(EXIT_FAILURE);
	}

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
		exit(1);
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
		logIssue(": unable to bind address TCP\n");
		exit(1);
	}
	/* Initiate the listen on the socket so remote users
	* can connect.  The listen backlog is set to 5, which
	* is the largest currently supported. */
	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
		logIssue("%s: unable to listen on socket\n");
		exit(1);
	}

	/* Create the socket UDP. */
	s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1) {
		logIssue("%s: unable to create socket UDP\n");
		exit(1);
	}
	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		logIssue("%s: unable to bind address UDP\n");
		exit(1);
	}

  	setpgrp();

	pid_t daemonPid;
	if(-1 == (daemonPid = fork())){
		fprintf(stderr,"\nUnable to fork daemon");
		exit(EXIT_FAILURE);
	}
	
	if(0 != daemonPid){
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
			fprintf(stderr,"\nClosing server because of signal recived in select");
			exit(EXIT_FAILURE);
		}


		/* Comprobamos si el socket seleccionado es el socket TCP */
		if (FD_ISSET(ls_TCP, &readmask)) {

			if(-1 ==(s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen))){
				fprintf(stderr,"\nUnable to accept tcp socket from parent");
				exit(EXIT_FAILURE);
			}

			pid_t serverInstancePid;
			if(-1 == (serverInstancePid = fork())){
				fprintf(stderr,"\nUnable to fork tcp server instance");
				close(ls_TCP);
				exit(EXIT_FAILURE);
			}
			if(0 == serverInstancePid){
//TODO tabulate -1
				close(ls_TCP); /* Close the listen socket inherited from the daemon. */
 				//obtain the host information to start communicacion with remote host
  					if( getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),hostName,sizeof(hostName),NULL,0,0) ){
 						if(NULL == inet_ntop(AF_INET,&clientaddr_in.sin_addr,hostName,sizeof(hostName))){
							fprintf(stderr,"\nError inet_ntop");
							close(ls_TCP);
							exit(EXIT_FAILURE);
						}
					}
					port = ntohs(clientaddr_in.sin_port);
					strcpy(hostIp, inet_ntoa(clientaddr_in.sin_addr));

 					//set opMode
  					lngr.l_onoff = 1;
					lngr.l_linger= 1;
					if(-1 == setsockopt(s_TCP,SOL_SOCKET,SO_LINGER,&lngr,sizeof(lngr))){
						fprintf(stderr,"\n%s",hostName);
						close(ls_TCP);
						exit(EXIT_FAILURE);
					}

					//read client request to know if is required read or write
					msgSize = recv(s_TCP,buffer,TAM_BUFFER,0);
					if(msgSize <= 0){
						msgSize = fillBufferWithErrMsg(UNKNOWN,"error on receiving client request", buffer);
                        if(send(s_TCP, buffer, msgSize, 0) != msgSize){
                            fprintf(stderr,"\nError on sending error msg");
                            close(ls_TCP);
                            exit(EXIT_FAILURE);
                        }
						logError(hostName, hostIp, requestmsg.fileName,"TCP", port, -1, "error on receiving client request");
						exit(EXIT_FAILURE);
					}

					requestmsg = fillReadMsgWithBuffer(buffer);
					//if the msg isnt a request send error 
					if(READ_TYPE != requestmsg.header && WRITE_TYPE != requestmsg.header){
 						msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,"error on client request header: isn't read or write", buffer);
                        if(send(s_TCP, buffer, msgSize, 0) != msgSize){
                            fprintf(stderr,"\nError on sending error for block");
                            close(ls_TCP);
                            exit(EXIT_FAILURE);
                        }
 						logError(hostName, hostIp, "","TCP", port,ILLEGAL_OPERATION, "error on client request header: isn't read or write");
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
			if(-1 == (msgSize = recvfrom(s_UDP, buffer, TAM_BUFFER, 0, (struct sockaddr*)&clientaddr_in, &addrlen))){
				fprintf(stderr,"\nRecvfrom error");
				exit(EXIT_FAILURE);
			}

			//create new port (efimero) (soket + bind)
			int s;
			struct sockaddr_in myaddrin;

			myaddrin.sin_family = AF_INET;
			myaddrin.sin_port = 0;
			myaddrin.sin_addr.s_addr = INADDR_ANY;

			/* Create the socket UDP. */
			s = socket (AF_INET, SOCK_DGRAM, 0);
			if (s == -1) {
				logIssue(" unable to create socket UDP\n");
				exit(1);
		    	}
			/* Bind the server's address to the socket. */
			if (bind(s, (struct sockaddr *) &myaddrin, sizeof(struct sockaddr_in)) == -1) {
				logIssue(" unable to bind address UDP\n");
				exit(1);
		      	}
		      	
		      	if(getsockname(s, (struct sockaddr *)&myaddrin, &addrlen) == -1){
				fprintf(stderr,"\nUnable to read socket address");
				exit(EXIT_FAILURE);
				}

			pid_t serverInstancePid;
			if((serverInstancePid = fork()) == -1){
				fprintf(stderr,"\nUnable to fork tcp server instance");
				exit(EXIT_FAILURE);
			}
			if(0 == serverInstancePid){
				requestmsg = fillReadMsgWithBuffer(buffer);
				//if the msg isnt a request send error 
				if(READ_TYPE != requestmsg.header && WRITE_TYPE != requestmsg.header){
 					msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,"error on client request header: isn't read or write", buffer);
                    if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                        fprintf(stderr,"\nError on sending error for block");
                        close(ls_TCP);
                        exit(EXIT_FAILURE);
                    }
					logError(hostName, hostIp, "","UDP", port,ILLEGAL_OPERATION, "error on client request header: isn't read or write");
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
	}

	close(s_UDP);
	close(ls_TCP);
	exit(EXIT_SUCCESS);
}

void SIGTERMHandler(int ss){
    end = TRUE;
}

void SIGALRMHandler(int ss){
	logError(SIGALRMHostName, SIGALRMHostIp, SIGALRMFileName,"UDP", SIGALRMPort, -1, "the timeout passed");
	exit(EXIT_SUCCESS);
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

	//save the parameters to log the timeout if it passes
	if(mode == UDP_MODE){
		strcpy(SIGALRMHostName,hostName);
		strcpy(SIGALRMHostIp,hostIp);
		strcpy(SIGALRMFileName,fileName);
		SIGALRMPort = port;
	}

	//log the connection
	logConnection(hostName, hostIp,fileName, MODE_STR(mode), port, 0, LOG_START_READ);

	//check if file exists
	bool fileExists = (access(fileName, F_OK ) != -1) ? TRUE : FALSE;

	//send error if file not found
	if(!fileExists){
		msgSize = fillBufferWithErrMsg(FILE_NOT_FOUND,"server coundn't found the requested file", buffer);
        if(mode==TCP_MODE){
            if(send(s, buffer, msgSize, 0) != msgSize){
                fprintf(stderr,"\nError on sending error for block");
                exit(EXIT_FAILURE);
            }
        }else{
            if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                fprintf(stderr,"\nError on sending error for block");
                exit(EXIT_FAILURE);
            }
        }
		logError(hostName, hostIp, fileName,MODE_STR(mode), port, FILE_NOT_FOUND, "server coundn't found the requested file");
		exit(EXIT_FAILURE);
	}

	//open the request file
	if(NULL == (f = fopen(fileName,"rb"))){
		//if error send error msg 
		msgSize = fillBufferWithErrMsg(UNKNOWN, "server couldn't open the file", buffer);
        if(mode==TCP_MODE){
            if(send(s, buffer, msgSize, 0) != msgSize){
                fprintf(stderr,"\nError on sending error for block");
                exit(EXIT_FAILURE);
            }
        }else{
            if(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize){
                fprintf(stderr,"\nError on sending error for block");
                exit(EXIT_FAILURE);
            }
        }
		logError(hostName, hostIp, fileName,MODE_STR(mode), port,-1, "server couldn't open file");
		exit(EXIT_FAILURE);
	}

	blockNumber = 0;
	endSesion = FALSE;
	while(!endSesion){
		//read data block from file
		memset(dataBuffer,0,MSG_DATA_SIZE);
		readSize = fread(dataBuffer, 1, MSG_DATA_SIZE, f);
		if(-1 == readSize){
			close(s);			
			fclose(f);			
			logError(hostName, hostIp, fileName,MODE_STR(mode), port,-1 , "Error reading the file");
			exit(EXIT_FAILURE);
		}

		//send the readed data block
		msgSize = fillBufferWithDataMsg(blockNumber,dataBuffer,readSize,buffer);
		if(mode == TCP_MODE){
			if(msgSize != send(s, buffer, msgSize, 0)){
				close(s);			
				fclose(f);			
				logError(hostName, hostIp, fileName,MODE_STR(mode), port,-1 , "Unable to send data block");
				exit(EXIT_FAILURE);
			}
		}else{
			if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in))){
				close(s);			
				fclose(f);			
				logError(hostName, hostIp, fileName,MODE_STR(mode), port,-1, "Unable to send data block");
				exit(EXIT_FAILURE);
			}
		}
		//wait for ack 
		if(mode  == TCP_MODE){
			msgSize = recv(s, buffer, TAM_BUFFER, 0);
			if(msgSize < TAM_BUFFER && msgSize>0) buffer[msgSize] = '\0';
		}else{    
			alarm(TIMEOUT);
			msgSize = recvfrom(s, buffer, TAM_BUFFER, 0,(struct sockaddr*)&clientaddr_in,&addrlen);
			if(msgSize < TAM_BUFFER) buffer[msgSize] = '\0';
		}	
		if(msgSize <= 0){
			close(s);			
			fclose(f);
			logError(hostName, hostIp, fileName,MODE_STR(mode), port,-1, "Unable to recive ack");
			exit(EXIT_FAILURE);	
		}

		switch(getMessageTypeWithBuffer(buffer)){
			case ACK_TYPE: 
				ackmsg = fillAckWithBuffer(buffer);
				//check if ack its correct; if not an error is send
				if( blockNumber != ackmsg.blockNumber ){
					close(s);			
					fclose(f);
					fprintf(stderr, "%d %d\n",blockNumber,ackmsg.blockNumber );
					logError(hostName, hostIp, fileName,MODE_STR(mode), port, -1 , "Recived ACK for block which doesn't matches with current");
					exit(EXIT_FAILURE);
				}
				blockNumber += 1;
			break;
			case ERR_TYPE: 
				errmsg = fillErrWithBuffer(buffer);
				close(s);			
				fclose(f);			
				logError(hostName, hostIp, fileName,MODE_STR(mode), port, errmsg.errorCode , errmsg.errorMsg);
				exit(EXIT_FAILURE);
			break;
			default:	
				close(s);			
				fclose(f);			
				logError(hostName, hostIp, fileName,MODE_STR(mode), port, -1, "Unrecognized operation in current context");
				exit(EXIT_FAILURE);
			break; 
		}

        
		//log received data 

		//check if the file has ended
		if(feof(f))
			endSesion = TRUE;
	}

	close(s);
	fclose(f);
	logConnection(hostName, hostIp,fileName, MODE_STR(mode), port, 0, LOG_END);
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

	//save the parameters to log the timeout if it passes
	if(mode == UDP_MODE){
		strcpy(SIGALRMHostName,hostName);
		strcpy(SIGALRMHostIp,hostIp);
		strcpy(SIGALRMFileName,fileName);
		SIGALRMPort = port;
	}

	//log the connection
	logConnection(hostName, hostIp,fileName, MODE_STR(mode), port, 0, LOG_START_READ);

	//check if file exists
	bool fileExists = (access( fileName, F_OK ) != -1) ? TRUE : FALSE;

	//send error if file alredy exsits
	/*if(fileExists){
		sprintf(tag,"the requested file alerady exists: %s" ,requestmsg.fileName);
		msgSize = fillBufferWithErrMsg(FILE_ALREADY_EXISTS,tag, buffer);
		EXIT_ON_WRONG_VALUE(TRUE,"Error becuase file exists",(send(s, buffer, msgSize, 0) != msgSize));
		logError(FILE_ALREADY_EXISTS, tag);
		exit(EXIT_FAILURE);
	}*/

	//open the file to write
	char finalPath[30];
	sprintf(finalPath,"%s/%s",READ_FILES_FOLDER,fileName);
	if(NULL == (f = fopen(finalPath,"wb"))){
		//if error send error msg 
		close(s);	
		logError(hostName, hostIp, fileName,MODE_STR(mode), port, -1, "client couldn't open file");
		exit(EXIT_FAILURE);
	}

	blockNumber = 0;
	//send ack and increment block
	msgSize = fillBufferWithAckMsg(blockNumber, buffer);
	if(mode == TCP_MODE){
		if(msgSize != send(s, buffer, msgSize, 0)){
			close(s);			
			fclose(f);			
			logError(hostName, hostIp, fileName,MODE_STR(mode), port, -1 , "Unable to send ack");
			exit(EXIT_FAILURE);
		}
	}else{
		if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in))){
			close(s);			
			fclose(f);			
			logError(hostName, hostIp, fileName,MODE_STR(mode), port, -1 , "Unable to send ack");
			exit(EXIT_FAILURE);
		}
	}

	endSesion = 0;
	while(!endSesion){

		//recive data block
		if(mode  == TCP_MODE){
			msgSize = recv(s, buffer, TAM_BUFFER, 0);
		  	if(msgSize < TAM_BUFFER && msgSize>0) buffer[msgSize] = '\0';
		}else{    
			alarm(TIMEOUT);
			msgSize = recvfrom(s, buffer, TAM_BUFFER, 0,(struct sockaddr*)&clientaddr_in,&addrlen);
			if(msgSize < TAM_BUFFER) buffer[msgSize] = '\0';
		}	
		if(msgSize <= 0){
			close(s);			
			fclose(f);
			logError(hostName, hostIp, fileName,MODE_STR(mode), port, -1 , "Unable to recive data block");
			exit(EXIT_FAILURE);	
		}

		//act according to type
		switch(getMessageTypeWithBuffer(buffer)){
			case DATA_TYPE:
        //increment block number 
        blockNumber += 1;
				datamsg = fillDataWithBuffer(msgSize,buffer);
				//check if block number is the correct one
				if(datamsg.blockNumber != blockNumber ){
					close(s);			
					fclose(f);
					logError(hostName, hostIp, fileName,MODE_STR(mode), port, -1 , "Recived data block which doesnt matches with current");
					exit(EXIT_FAILURE);
				}
				//write the send data
				writeResult = fwrite(datamsg.data,1,DATA_SIZE(msgSize),f);
				if(-1 == writeResult){
					close(s);			
					fclose(f);
					logError(hostName, hostIp, fileName,MODE_STR(mode), port, -1 , "Unable to write the file");
					exit(EXIT_FAILURE);
				}
				//check if the block is the last -- size less than 512 bytes
				if(DATA_SIZE(msgSize) < MSG_DATA_SIZE)
					endSesion = TRUE;
			break;
			case ERR_TYPE: 
				errmsg = fillErrWithBuffer(buffer);
				close(s);			
				fclose(f);			
				logError(hostName, hostIp, fileName,MODE_STR(mode), port, errmsg.errorCode , errmsg.errorMsg);
				exit(EXIT_FAILURE);
			break;
			default:	
				close(s);			
				fclose(f);			
				logError(hostName, hostIp, fileName,MODE_STR(mode), port, -1 , "Unrecognized operation in current context");
				exit(EXIT_FAILURE);
			break; 
		}

		//send ack
		msgSize = fillBufferWithAckMsg(blockNumber, buffer);
		if(mode == TCP_MODE){
			if(msgSize != send(s, buffer, msgSize, 0)){
				close(s);			
				fclose(f);			
				logError(hostName, hostIp, fileName,MODE_STR(mode), port, -1 , "Unable to send ack");
				exit(EXIT_FAILURE);
			}
		}else{
			if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in))){
				close(s);			
				fclose(f);			
				logError(hostName, hostIp, fileName,MODE_STR(mode), port, -1 , "Unable to send ack");
				exit(EXIT_FAILURE);
			}
		}
				
		//log received data 
		logConnection(hostName, hostIp,fileName, MODE_STR(mode), port, datamsg.blockNumber, LOG_READ);
	}

	close(s);
	fclose(f);
	logConnection(hostName, hostIp,fileName, MODE_STR(mode), port, 0, LOG_END);
}




void logConnection(char * hostName, char * hostIp, char * fileName, char * protocol, int port, int block, int mode){
	char path[20];
	char toLog[LOG_MESSAGE_SIZE];

	sprintf(path,"%s/%s",LOG_FOLDER,SERVER_LOG);
	FILE*logFile = fopen(path,"a+");

	switch(mode){
		case LOG_START_READ:  
			sprintf(toLog,"\n[%s][HOST: %s][IP: %s][PROTOCOL: %s][PORT: %d][FILE: %s][STARTED READ]", getDateAndTime(), hostName, hostIp, protocol, port, fileName); 
		break;
		case LOG_START_WRITE: 
			sprintf(toLog,"\n[%s][HOST: %s][IP: %s][PROTOCOL: %s][PORT: %d][FILE: %s][STARTED WRITE]", getDateAndTime(), hostName, hostIp, protocol, port, fileName); 
		break;
		case LOG_READ:        
			sprintf(toLog,"\n[%s][HOST: %s][IP: %s][PROTOCOL: %s][PORT: %d][FILE: %s][RECIVED: %d]", getDateAndTime(), hostName, hostIp, protocol, port, fileName, block); 
		break;
		case LOG_WRITE:       
			sprintf(toLog,"\n[%s][HOST: %s][IP: %s][PROTOCOL: %s][PORT: %d][FILE: %s][SEND: %d]", getDateAndTime(), hostName, hostIp, protocol, port, fileName, block); 
		break;
		case LOG_END:         
			sprintf(toLog,"\n[%s][HOST: %s][IP: %s][PROTOCOL: %s][PORT: %d][FILE: %s][SUCCED]", getDateAndTime(), hostName, hostIp, protocol, port, fileName); 
		break;
	}

	fprintf(logFile,"%s",toLog);

	fclose(logFile);
}

void logError(char * hostName, char * hostIp, char * fileName, char * protocol, int port, int errorCode, char * errormsg){
	char path[40];
	char error[30];
	char toLog[LOG_MESSAGE_SIZE];

	sprintf(path,"%s/%s",LOG_FOLDER,SERVER_LOG);
	FILE*logFile = fopen(path,"a+");    

	switch (errorCode) {
		case 0: strcpy(error,"UNKNOWN"); break;
		case 1: strcpy(error,"FILE_NOT_FOUND"); break;
		case 3: strcpy(error,"DISK_FULL"); break;
		case 4: strcpy(error,"ILEGAL_OPERATION"); break;
		case 6: strcpy(error,"FILE_ALREADY_EXISTS"); break;
		case -1: strcpy(error,"-1"); break;
	}

	sprintf(toLog,"\n[%s][HOST: %s][IP: %s][PROTOCOL: %s][FILE: %s][ERROR [CODE: %s] [MSG: %s] ]",getDateAndTime(), hostName, hostIp, protocol, fileName, error, errormsg);

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
