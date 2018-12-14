/*

** Fichero: TFTPclient.c
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
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "utils.h"
#include "msgUtils.h"

#define USAGE_ERROR_MSG "Error: usage ./client <server name or ip> <TCP or UDP> <l or e> <file>"

#define TCP_ARG "TCP"
#define UDP_ARG "UDP"
#define READ_ARG "l"
#define WRITE_ARG "e"

bool timeOutPassed;
int nRetries;

char SIGALRMHostName[30];
char SIGALRMFileName[30];
int SIGALRMPort;
int SIGALRMs;
int SIGALRMs;
FILE* SIGALRMf;
void SIGALRMHandler(int ss);

void TFTPclientReadMode(ProtocolMode mode,char * hostName, char * file);
void TFTPclientWriteMode(ProtocolMode mode,char * hostName, char * file);

const char * getDateAndTime();
void logError(char * hostName,int port, char * fileName, char * protocol, int errorCode, char * errormsg);
void logConnection(char * hostName,int port, char * fileName, char * protocol, int block, int mode);

int main(int argc, char * argv[]){
	sigset_t signalSet;
	ProtocolMode mode;

	//Check the args are correct
	if( argc != 5
		|| !(!strcmp(argv[2],TCP_ARG) || !strcmp(argv[2],UDP_ARG))
		|| !(!strcmp(argv[3],READ_ARG)|| !strcmp(argv[3],WRITE_ARG))
	){
		fprintf(stderr,"%s",USAGE_ERROR_MSG);
		exit(EXIT_FAILURE);
	}

	//check if file exists and throw error if neccesary
	char finalPath[30];
	sprintf(finalPath,"%s/%s",CLIENT_FILES_FOLDER,argv[4]);
	bool fileExists = (access( finalPath, F_OK ) != -1) ? TRUE : FALSE;
	if(fileExists && !strcmp(argv[3],READ_ARG)){
		fprintf(stderr,"\nIs read mode and the given file (%s) already exists",argv[4]);
		exit(EXIT_FAILURE);
	}else if(!fileExists && !strcmp(argv[3],WRITE_ARG)){
		fprintf(stderr,"\nIs write mode and the given file (%s) doesn't exists",argv[4]);
		exit(EXIT_FAILURE);
	}

	mode = (!strcmp(TCP_ARG,argv[2]))? TCP_MODE : UDP_MODE;

	//register signals
	if(mode == UDP_MODE){
		struct sigaction ss;
 		memset(&ss,0,sizeof(ss));

 		ss.sa_handler=SIGALRMHandler;
 		ss.sa_flags=0;

 		if(-1 == sigfillset(&ss.sa_mask)
 			|| -1 == sigaction(SIGALRM,&ss,NULL)
 		){
			fprintf(stderr,"Unable to make the signal redefinition");
			exit(EXIT_FAILURE);
 		}
	}
	if(-1 == sigfillset(&signalSet) ){
		fprintf(stderr,"Unable to make the signal redefinition");
		exit(EXIT_FAILURE);
	}
	if(-1 == sigdelset(&signalSet,SIGTERM)){
		fprintf(stderr,"Unable to make the signal redefinition");
		exit(EXIT_FAILURE);
	}
	if( mode == UDP_MODE && -1 == sigdelset(&signalSet,SIGALRM)){
		fprintf(stderr,"Unable to make the signal redefinition");
		exit(EXIT_FAILURE);
	}
	if(-1 == sigprocmask(SIG_SETMASK, &signalSet, NULL)){
		fprintf(stderr,"Unable to make the signal redefinition");
		exit(EXIT_FAILURE);
	}

	//go to protocol
	if(!strcmp(argv[3],READ_ARG)){
		TFTPclientReadMode(mode,argv[1], argv[4]);
	}else{
		TFTPclientWriteMode(mode,argv[1], argv[4]);
	}

	return 0;
}

void SIGALRMHandler(int ss){
	if(nRetries<RETRIES){
		nRetries++;
		timeOutPassed = TRUE;
	}else{
		logError(SIGALRMHostName,SIGALRMPort,SIGALRMFileName, "UDP", UNKNOWN, "Timeout passed");
		close(SIGALRMs);
		if(NULL != SIGALRMf)
			fclose(SIGALRMf);
		exit(EXIT_SUCCESS);
	}
}

void TFTPclientReadMode(ProtocolMode mode,char * hostName, char * file){
	int s, addrlen, errcode;				
	struct addrinfo hints, *res;
	struct sockaddr_in myaddr_in;	    
	struct sockaddr_in servaddr_in;
	struct in_addr reqaddr; 	

	bool endSesion = FALSE;

	rwMsg requestMsg;
	dataMsg datamsg;
	ackMsg ackmsg;
	errMsg errmsg;
	headers msgType;

	FILE * f = NULL;
	int blockNumber , port;
	int msgSize, writeResult, readSize;
	char dataBuffer[MSG_DATA_SIZE];
	char buffer[TAM_BUFFER];	

	addrlen = sizeof(struct sockaddr_in);

	if(mode == TCP_MODE){
		/* Create the socket. */
		if(-1 == (s = socket (AF_INET, SOCK_STREAM, 0))){
			fprintf(stderr,"\nUnable to create the socket");
			exit(EXIT_FAILURE);
		}
		/* clear out address structures */
		memset (&myaddr_in, 0, sizeof(struct sockaddr_in));
		memset (&servaddr_in, 0, sizeof(struct sockaddr_in));
		/* Set up the peer address to which we will connect. */
		servaddr_in.sin_family = AF_INET;
		/* Get the host information for the hostname that the
		 * user passed in. */
		memset (&hints, 0, sizeof (hints));
		hints.ai_family = AF_INET;
	 	 /* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
		if(0 != getaddrinfo (hostName, NULL, &hints, &res)){
			fprintf(stderr,"\nNot possible to solve IP");
			exit(EXIT_FAILURE);
		}
		/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	 	freeaddrinfo(res);
		servaddr_in.sin_port = htons(PORT);
		if(-1 == connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in))){
			fprintf(stderr,"\nUnable to connect to remote");
			exit(EXIT_FAILURE);
		}
		//returns the information of our socket
		if(-1 == getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen)){
			fprintf(stderr,"\nUnable to read socket address");
			exit(EXIT_FAILURE);
		}

		port = ntohs(myaddr_in.sin_port);

	}else{
		/* Create the socket. */
		if(-1 == (s = socket (AF_INET, SOCK_DGRAM, 0))){
			fprintf(stderr, "\nUnable to create socket");
			exit(EXIT_FAILURE);
		}

		/* clear out address structures */
		memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
		memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
	  
		/* Bind socket to some local address so that the
		* server can send the reply back */
		myaddr_in.sin_family = AF_INET;
		myaddr_in.sin_port = 0;
		myaddr_in.sin_addr.s_addr = INADDR_ANY;
		if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
			fprintf(stderr, "\nUnable to bind socket\n");
			exit(EXIT_FAILURE);
		}
		if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
			fprintf(stderr, "\nUnable to read socket address\n");
			exit(EXIT_FAILURE);
		}

		/* Set up the server address. */
		servaddr_in.sin_family = AF_INET;
		/* Get the host information for the server's hostname that the
		* user passed in.*/
		memset (&hints, 0, sizeof (hints));
		hints.ai_family = AF_INET;
		/* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
		if( 0 != getaddrinfo (hostName, NULL, &hints, &res)){
			/* Name was not found.  Return a
			* special value signifying the error. */
			fprintf(stderr, "\nNot possible to solve IP");
			exit(EXIT_FAILURE);
		}

		/* Copy address of host */
		port = ntohs(myaddr_in.sin_port);
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
		
		freeaddrinfo(res);
		/* puerto del servidor en orden de red*/
		servaddr_in.sin_port = htons(PORT);
	}

	//save the parameters to log the timeout if it passes
	if(mode == UDP_MODE){
		strcpy(SIGALRMHostName,hostName);
		strcpy(SIGALRMFileName,file);
		SIGALRMPort = ntohs(myaddr_in.sin_port);
		SIGALRMf = NULL;
		SIGALRMs = s;
	}

	//log the connection
	logConnection(hostName, port, file, MODE_STR(mode), port, LOG_START_READ);
		
	//send request to start protocol and wait for first data block 
	if(mode == TCP_MODE){
		//send request
		msgSize = fillBufferWithReadMsg(TRUE,file, buffer);
		if(msgSize != send(s, buffer, msgSize, 0)){
			close(s);			
			fclose(f);
			logError(hostName, port, file, "TCP", -1 , "Unable to send first request");
			exit(EXIT_FAILURE);	
		}
		//wait for data block
		memset(buffer,0,TAM_BUFFER);
		msgSize = recv(s, buffer, TAM_BUFFER, 0);
		if(msgSize < TAM_BUFFER && msgSize>0) buffer[msgSize] = '\0';
	}else{
		nRetries = 0;
		do{
			//send request
			msgSize = fillBufferWithReadMsg(TRUE,file, buffer);
			if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) ){
				close(s);	
				logError(hostName, port, file, "UDP", -1 , "Unable to send first request");
				exit(EXIT_FAILURE);	
			}
			//wait for data block
			memset(buffer,0,TAM_BUFFER);
			timeOutPassed = FALSE;
			alarm(TIMEOUT);
			msgSize = recvfrom(s, buffer, TAM_BUFFER, 0,(struct sockaddr*)&servaddr_in,&addrlen);
			if(msgSize < TAM_BUFFER) buffer[msgSize] = '\0';
		}while(timeOutPassed);
		alarm(0);
	}
	if(msgSize <= 0){
		close(s);	
		logError(hostName, port, file, MODE_STR(mode), -1 , "Unable to recive data block");
		exit(EXIT_FAILURE);	
	}

	//open the file to write
	char finalPath[30];
	sprintf(finalPath,"%s/%s",CLIENT_FILES_FOLDER,file);
	if(NULL == (f = fopen(finalPath,"wb"))){
		//if error send error msg 
		close(s);	
		logError(hostName, port, file, MODE_STR(mode), -1, "client couldn't open file");
		exit(EXIT_FAILURE);
	}

	if(mode == UDP_MODE){
		SIGALRMf = f;
	}

	endSesion = FALSE;
	blockNumber = 1;
	while(!endSesion){
		//act according to type
		switch(getMessageTypeWithBuffer(buffer)){
			case DATA_TYPE:
				datamsg = fillDataWithBuffer(msgSize,buffer);
				//check if block number is the correct one
				if(datamsg.blockNumber != blockNumber ){
					close(s);			
					fclose(f);
					logError(hostName, port, file, MODE_STR(mode), -1 , "Recived data block which doesnt matches with current");
					exit(EXIT_FAILURE);
				}
				//write the send data
				writeResult = fwrite(datamsg.data,1,DATA_SIZE(msgSize),f);
				if(-1 == writeResult){
					close(s);			
					fclose(f);
					logError(hostName, port, file, MODE_STR(mode), -1 , "Unable to write the file");
					exit(EXIT_FAILURE);
				}
				//check if the block is the last -- size less than 512 bytes
				if(DATA_SIZE(msgSize) < MSG_DATA_SIZE)
					endSesion = TRUE;
				//increment block number 
				blockNumber += 1;
			break;
			case ERR_TYPE: 
				errmsg = fillErrWithBuffer(buffer);
				close(s);			
				fclose(f);			
				logError(hostName, port, file, MODE_STR(mode), errmsg.errorCode , errmsg.errorMsg);
				exit(EXIT_FAILURE);
			break;
			default:	
				close(s);			
				fclose(f);			
				logError(hostName, port, file, MODE_STR(mode), ILLEGAL_OPERATION, "Unrecognized operation in current context");
				exit(EXIT_FAILURE);
			break; 
		}

		if(!endSesion){
			//send ack and recive next datablock
			if(mode == TCP_MODE){
				//send ack
				msgSize = fillBufferWithAckMsg(datamsg.blockNumber, buffer);
				if(msgSize != send(s, buffer, msgSize, 0)){
					close(s);			
					fclose(f);			
					logError(hostName, port, file,"TCP", -1 , "Unable to send ack");
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
					msgSize = fillBufferWithAckMsg(datamsg.blockNumber, buffer);
					if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in))){
						close(s);			
						fclose(f);			
						logError(hostName, port, file,"UDP", -1 , "Unable to send ack");
						exit(EXIT_FAILURE);
					}
					//recive next data block
					memset(buffer,0,TAM_BUFFER);
					timeOutPassed = FALSE;
					alarm(TIMEOUT);
					msgSize = recvfrom(s, buffer, TAM_BUFFER, 0,(struct sockaddr*)&servaddr_in,&addrlen);
					if(msgSize < TAM_BUFFER) buffer[msgSize] = '\0';
				}while(timeOutPassed);
			}
			
			if(msgSize <= 0){
				close(s);			
				fclose(f);
				logError(hostName, port, file, MODE_STR(mode), -1 , "Unable to recive data block");
				exit(EXIT_FAILURE);	
			}

			//log received data 
			logConnection(hostName, port, file, MODE_STR(mode), datamsg.blockNumber, LOG_READ);
		}

	}

	//send last ack
	if(mode == TCP_MODE){
		msgSize = fillBufferWithAckMsg(datamsg.blockNumber, buffer);
		if(msgSize != send(s, buffer, msgSize, 0)){
			close(s);			
			fclose(f);			
			logError(hostName, port, file,"TCP", -1 , "Unable to send ack");
			exit(EXIT_FAILURE);
		}
	}else{
		msgSize = fillBufferWithAckMsg(datamsg.blockNumber, buffer);
		if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in))){
			close(s);			
			fclose(f);			
			logError(hostName, port, file,"UDP", -1 , "Unable to send ack");
			exit(EXIT_FAILURE);
		}
	}


	close(s);
	fclose(f);

	logConnection(hostName, port, file, MODE_STR(mode), 0, LOG_END);
}

void TFTPclientWriteMode(ProtocolMode mode,char * hostName, char * file){
	int s, addrlen, errcode;				
	struct addrinfo hints, *res;
	struct sockaddr_in myaddr_in;	    
	struct sockaddr_in servaddr_in;
	struct in_addr reqaddr; 	

	bool endSesion = FALSE;

	rwMsg requestMsg;
	dataMsg datamsg;
	ackMsg ackmsg;
	errMsg errmsg;
	headers msgType;

	FILE * f = NULL;
	int blockNumber , port;
	int msgSize, writeResult, readSize;
	char dataBuffer[MSG_DATA_SIZE];
	char buffer[TAM_BUFFER];	

	addrlen = sizeof(struct sockaddr_in);

	if(mode == TCP_MODE){
		/* Create the socket. */
		if(-1 == (s = socket (AF_INET, SOCK_STREAM, 0))){
			fprintf(stderr,"\nUnable to create the socket");
			exit(EXIT_FAILURE);
		}
		/* clear out address structures */
		memset (&myaddr_in, 0, sizeof(struct sockaddr_in));
		memset (&servaddr_in, 0, sizeof(struct sockaddr_in));
		/* Set up the peer address to which we will connect. */
		servaddr_in.sin_family = AF_INET;
		/* Get the host information for the hostname that the
		 * user passed in. */
		memset (&hints, 0, sizeof (hints));
		hints.ai_family = AF_INET;
	 	 /* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
		if(0 != getaddrinfo (hostName, NULL, &hints, &res)){
			fprintf(stderr,"\nNot possible to solve IP");
			exit(EXIT_FAILURE);
		}
		/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	 	freeaddrinfo(res);
		servaddr_in.sin_port = htons(PORT);
		if(-1 == connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in))){
			fprintf(stderr,"\nUnable to connect to remote");
			exit(EXIT_FAILURE);
		}
		//returns the information of our socket
		if(-1 == getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen)){
			fprintf(stderr,"\nUnable to read socket address");
			exit(EXIT_FAILURE);
		}

		port = ntohs(myaddr_in.sin_port);

	}else{
		/* Create the socket. */
		if(-1 == (s = socket (AF_INET, SOCK_DGRAM, 0))){
			fprintf(stderr, "\nUnable to create socket");
			exit(EXIT_FAILURE);
		}

		/* clear out address structures */
		memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
		memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
	  
		/* Bind socket to some local address so that the
		* server can send the reply back */
		myaddr_in.sin_family = AF_INET;
		myaddr_in.sin_port = 0;
		myaddr_in.sin_addr.s_addr = INADDR_ANY;
		if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
			fprintf(stderr, "\nUnable to bind socket\n");
			exit(EXIT_FAILURE);
		}
		if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
			fprintf(stderr, "\nUnable to read socket address\n");
			exit(EXIT_FAILURE);
		}

		/* Set up the server address. */
		servaddr_in.sin_family = AF_INET;
		/* Get the host information for the server's hostname that the
		* user passed in.*/
		memset (&hints, 0, sizeof (hints));
		hints.ai_family = AF_INET;
		/* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
		if( 0 != getaddrinfo (hostName, NULL, &hints, &res)){
			/* Name was not found.  Return a
			* special value signifying the error. */
			fprintf(stderr, "\nNot possible to solve IP");
			exit(EXIT_FAILURE);
		}

		/* Copy address of host */
		port = ntohs(myaddr_in.sin_port);
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
		
		freeaddrinfo(res);
		/* puerto del servidor en orden de red*/
		servaddr_in.sin_port = htons(PORT);
	}

	//save the parameters to log the timeout if it passes
	if(mode == UDP_MODE){
		strcpy(SIGALRMHostName,hostName);
		strcpy(SIGALRMFileName,file);
		SIGALRMPort = ntohs(myaddr_in.sin_port);
		SIGALRMf = NULL;
		SIGALRMs = s;
	}

	//log the connection
	logConnection(hostName, port, file, MODE_STR(mode), port, LOG_START_WRITE);
		
	//send request to start protocol and wait for ack 0
	if(mode == TCP_MODE){
		//send request 
		msgSize = fillBufferWithReadMsg(FALSE,file, buffer);
		if(msgSize != send(s, buffer, msgSize, 0)){
			close(s);			
			fclose(f);
			logError(hostName, port, file, "TCP", -1 , "Unable to send first request");
			exit(EXIT_FAILURE);	
		}
		//wait for ack
		memset(buffer,0,TAM_BUFFER);
		msgSize = recv(s, buffer, TAM_BUFFER, 0);
		if(msgSize < TAM_BUFFER && msgSize>0) buffer[msgSize] = '\0';
	}else{
		nRetries = 0;
		do{
			//send request
			msgSize = fillBufferWithReadMsg(FALSE,file, buffer);
			if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) ){
				close(s);			
				fclose(f);
				logError(hostName, port, file, "UDP", -1 , "Unable to send first request");
				exit(EXIT_FAILURE);	
			}
			//wait for ack
			memset(buffer,0,TAM_BUFFER);
			timeOutPassed = FALSE;
			alarm(TIMEOUT);
			msgSize = recvfrom(s, buffer, TAM_BUFFER, 0,(struct sockaddr*)&servaddr_in,&addrlen);
			if(msgSize < TAM_BUFFER) buffer[msgSize] = '\0';
		}while(timeOutPassed);
		alarm(0);
	}
	if(msgSize <= 0){
		close(s);			
		fclose(f);
		logError(hostName, port, file, MODE_STR(mode), -1 , "Unable to recive ack");
		exit(EXIT_FAILURE);	
	}

	//open the file to write
	char finalPath[30];
	sprintf(finalPath,"%s/%s",CLIENT_FILES_FOLDER,file);
	if(NULL == (f = fopen(finalPath,"rb"))){
		//if error send error msg 
		close(s);	
		logError(hostName, port, file, MODE_STR(mode), -1, "client couldn't open file");
		exit(EXIT_FAILURE);
	}

	if(mode == UDP_MODE){
		SIGALRMf = f;
	}

	blockNumber = 0;
	endSesion = FALSE;
	while(!endSesion){
		switch(getMessageTypeWithBuffer(buffer)){
			case ACK_TYPE: 
				ackmsg = fillAckWithBuffer(buffer);
				//check if ack its correct; if not an error is send
				if( blockNumber != ackmsg.blockNumber ){
					close(s);			
					fclose(f);
					logError(hostName, port, file, MODE_STR(mode), -1 , "Recived ACK for block which doesn't matches with current");
					exit(EXIT_FAILURE);
				}
				blockNumber += 1;
			break;
			case ERR_TYPE: 
				errmsg = fillErrWithBuffer(buffer);
				close(s);			
				fclose(f);			
				logError(hostName, port, file, MODE_STR(mode), errmsg.errorCode , errmsg.errorMsg);
				exit(EXIT_FAILURE);
			break;
			default:	
				close(s);			
				fclose(f);			
				logError(hostName, port, file, MODE_STR(mode), getMessageTypeWithBuffer(buffer) , "Unrecognized operation in current context");
				exit(EXIT_FAILURE);
			break; 
		}

        //read data block from file
        memset(dataBuffer,0,MSG_DATA_SIZE);
        readSize = fread(dataBuffer, 1, MSG_DATA_SIZE, f);
        if(-1 == readSize){
			close(s);			
			fclose(f);			
			logError(hostName, port, file,MODE_STR(mode), -1 , "Error reading the file");
			exit(EXIT_FAILURE);
        }

        //send readed data block and wait for ack
        if(mode == TCP_MODE){
        	//send readed data block 
        	msgSize = fillBufferWithDataMsg(blockNumber,dataBuffer,readSize,buffer);
			if(msgSize != send(s, buffer, msgSize, 0)){
				close(s);			
				fclose(f);			
				logError(hostName, port, file,"TCP", -1 , "Unable to send data block");
				exit(EXIT_FAILURE);
			}
        	//wait for ack
			memset(buffer,0,TAM_BUFFER);
			msgSize = recv(s, buffer, TAM_BUFFER, 0);
			if(msgSize < TAM_BUFFER && msgSize>0) buffer[msgSize] = '\0';
        }else{
        	nRetries = 0;
        	do{
	        	//send readed data block 
	        	msgSize = fillBufferWithDataMsg(blockNumber,dataBuffer,readSize,buffer);
	        	if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in))){
					close(s);			
					fclose(f);			
					logError(hostName, port, file,"UDP", -1 , "Unable to send data block");
					exit(EXIT_FAILURE);
				}
	        	//wait for ack
				memset(buffer,0,TAM_BUFFER);
	        	timeOutPassed = FALSE;
				alarm(TIMEOUT);
				msgSize = recvfrom(s, buffer, TAM_BUFFER, 0,(struct sockaddr*)&servaddr_in,&addrlen);
				if(msgSize < TAM_BUFFER && msgSize>0) buffer[msgSize] = '\0';
			}while(timeOutPassed);
			alarm(0);
        }	
		if(msgSize <= 0){
			close(s);			
			fclose(f);
			logError(hostName, port, file, MODE_STR(mode), -1 , "Unable to recive ack");
			exit(EXIT_FAILURE);	
		}

		//log received data 
		logConnection(hostName, port, file, MODE_STR(mode), blockNumber, LOG_WRITE);

		//check if the file has ended
        if(feof(f))
          endSesion = TRUE;
	}

	close(s);
	fclose(f);
	logConnection(hostName, port, file, MODE_STR(mode), 0, LOG_END);
}


void logConnection(char * hostName,int port, char * fileName, char * protocol, int block, int mode){
	char path[20];
	char toLog[LOG_MESSAGE_SIZE];

	sprintf(path,"%d%s",port,CLIENT_LOG_EXT);
	FILE*logFile = fopen(path,"a+");

	switch(mode){
		case LOG_START_READ:  
			sprintf(toLog,"\n[%s][HOST: %s][PROTOCOL: %s][FILE: %s][STARTED READ]", getDateAndTime(), hostName, protocol, fileName); 
		break;
		case LOG_START_WRITE: 
			sprintf(toLog,"\n[%s][HOST: %s][PROTOCOL: %s][FILE: %s][STARTED WRITE]", getDateAndTime(), hostName, protocol, fileName); 
		break;
		case LOG_READ:        
			sprintf(toLog,"\n[%s][HOST: %s][PROTOCOL: %s][FILE: %s][RECIVED: %d]", getDateAndTime(), hostName, protocol, fileName, block); 
		break;
		case LOG_WRITE:       
			sprintf(toLog,"\n[%s][HOST: %s][PROTOCOL: %s][FILE: %s][SEND: %d]", getDateAndTime(), hostName, protocol, fileName, block); 
		break;
		case LOG_END:         
			sprintf(toLog,"\n[%s][HOST: %s][PROTOCOL: %s][FILE: %s][SUCCED]", getDateAndTime(), hostName, protocol, fileName); 
		break;
	}

	//fprintf(stderr,"%s",toLog);
	fprintf(logFile,"%s",toLog);

	fclose(logFile);
}

void logError(char * hostName, int port, char * fileName, char * protocol, int errorCode, char * errormsg){
	char path[40];
	char error[30];
	char toLog[LOG_MESSAGE_SIZE];

	sprintf(path,"%d%s",port,CLIENT_LOG_EXT);
	FILE*logFile = fopen(path,"a+");    

	switch (errorCode) {
		case 0: strcpy(error,"UNKNOWN"); break;
		case 1: strcpy(error,"FILE_NOT_FOUND"); break;
		case 3: strcpy(error,"DISK_FULL"); break;
		case 4: strcpy(error,"ILEGAL_OPERATION"); break;
		case 6: strcpy(error,"FILE_ALREADY_EXISTS"); break;
		case -1: strcpy(error,"INTERNAL_ERROR"); break;
	}

	sprintf(toLog,"\n[%s][HOST: %s][PROTOCOL: %s][PORT: %d][FILE: %s][ERROR: %s][MSG: %s]",getDateAndTime(), hostName, protocol, port, fileName, error, errormsg);
	fprintf(stderr,"%s",toLog);
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
