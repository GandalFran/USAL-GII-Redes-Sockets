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
#include "utils/utils.h"
#include "utils/msgUtils.h"

#define USAGE_ERROR_MSG "Error: usage ./client <server name or ip> <TCP or UDP> <r or w> <file>"

#define TCP_ARG "TCP"
#define UDP_ARG "UDP"
#define READ_ARG "r"
#define WRITE_ARG "w"

typedef enum ProtocolMode{TCP_MODE, UDP_MODE};

void TFTPclientReadMode(ProtocolMode mode,char * hostName, char * file);
void TFTPclientWriteMode(ProtocolMode mode,char * hostName, char * file);

const char * getDateAndTime();
void logError(char * hostName, int port, char * fileName, char * protocol, int errorCode, char * errormsg);
void logConnection(char * hostName, int port, char * fileName, char * protocol, int block, int mode);

int main(int argc, char * argv[]){
	//register signals

	//Check the args are correct
	if( argc != 5
		|| !(!strcmp(argv[2],TCP_ARG) || !strcmp(argv[2],UDP_ARG))
		|| !(!strcmp(argv[3],READ_ARG)|| !strcmp(argv[3],WRITE_ARG))
	){
		fprintf(stderr,"%s",USAGE_ERROR_MSG);
		exit(EXIT_FAILURE);
	}

	if(!strcmp(argv[2],TCP_ARG)){
		tcpClient(!strcmp(READ_ARG,argv[3]),argv[1],argv[4]);
	}else{
		udpClient(!strcmp(READ_ARG,argv[3]),argv[1],argv[4]);
	}

	return 0;
}

void SIGALRMHandler(int ss){
  logError();
  exit(EXIT_SUCCESS);
}

void TFTPclientReadMode(ProtocolMode mode,char * hostName, char * file){
	int s, addrlen, errcode;				
	struct addrinfo hints, *res;
	struct sockaddr_in myaddr_in;	    
	struct sockaddr_in servaddr_in;
	struct in_addr reqaddr; 	

	bool endSesion;

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
	
		/* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
		redefineSignal(SIGALRM,SIGALRMHandler);
	}


	//log the connection
	if(mode == TCP_MODE)
		logConnection(hostName, port, file, "TCP", 0, LOG_START_READ);
	else
		logConnection(hostName, port, file, "UDP", 0, LOG_START_READ);
		

	//send request to start protocol
	msgSize = fillBufferWithReadMsg(isReadMode,file, buffer);
	if(mode == TCP_MODE){
		if(msgSize != send(s, buffer, msgSize, 0)){
			close(s);			
			fclose(f);
			logError(hostName, port, fileName, "TCP", -1 , "Unable to send first request");
			exit(EXIT_FAILURE);	
		}
	}else{
		if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) ){
			close(s);			
			fclose(f);
			logError(hostName, port, fileName, "UDP", -1 , "Unable to send first request");
			exit(EXIT_FAILURE);	
		}
	}

	//check if file exists
	bool fileExists = (access( file, F_OK ) != -1) ? TRUE : FALSE;

	//open the file to write
	char finalPath[30];
	sprintf(finalPath,"%s/%s",READ_FILES_FOLDER,file);
	if(NULL == (f = fopen(finalPath,"wb"))){
	        //if error send error msg 
		close(s);			
		fclose(f);
 		sprintf(tag,"client couln't open %s" ,file);
		logError(hostName,port,UNKNOWN, tag);
		exit(EXIT_FAILURE);
	}

	while(!endSesion){
		//recive first one block
		if(mode  == TCP_MODE){
			msgSize = recv(s, buffer, TAM_BUFFER, 0);
		  	if(msgSize< TAM_BUFFER && msgSize>0) buffer[j] = '\0';
		}else{    
			alarm(TIMEOUT);
			msgSize = recvfrom(s, buffer, TAM_BUFFER, 0,(struct sockaddr*)&servaddr_in,&addrlen);
			if(j< TAM_BUFFER) buffer[j] = '\0';
		}	
		if(msgSize < 0){
			close(s);			
			fclose(f);
			logError(hostName, port, fileName, (mode == TCP_MODE)? "TCP":"UDP", -1 , "Unable to recive data block");
			exit(EXIT_FAILURE);	
		}

		//act according to type
		switch(getMessageTypeWithBuffer(buffer)){
			case DATA_TYPE:
				datamsg = fillDataWithBuffer(msgSize,buffer);
				//check if block number is the correct one
				if(datamsg.blockNumber != blockNumber ){
					close(s);			
					fclose(f);
					logError(hostName, port, fileName, (mode == TCP_MODE)? "TCP":"UDP", -1 , "Recived data block which doesnt matches with current");
					exit(EXIT_FAILURE);
				}
				//write the send data
				writeResult = fwrite(datamsg.data,1,DATA_SIZE(msgSize),f);
				if(-1 == writeResult){
					close(s);			
					fclose(f);
					logError(hostName, port, fileName, (mode == TCP_MODE)? "TCP":"UDP", -1 , "Unable to write the file");
					exit(EXIT_FAILURE);
				}
				//check if the block is the last -- size less than 512 bytes
				if(DATA_SIZE(msgSize) < MSG_DATA_SIZE)
					endSesion = TRUE;
				//increment block number 
				blockNumber += 1;
			break;
			case ERR_TYPE: 
				errmsg = fillErrMsgWithBuffer();
				close(s);			
				fclose(f);			
				logError(hostName, port, fileName, (mode == TCP_MODE)? "TCP":"UDP", errmsg.errorCode , errmsg.errMsg);
	                	exit(EXIT_FAILURE);
			break;
			default:	
				close(s);			
				fclose(f);			
				logError(hostName, port, fileName, (mode == TCP_MODE)? "TCP":"UDP", -1 , "Unrecognized operation in current context");
	                	exit(EXIT_FAILURE);
			break; 
		}

		//send ack
		msgSize = fillBufferWithAckMsg(datamsg.blockNumber, buffer);
		if(mode == TCP_MODE){
			if(msgSize != send(s, buffer, msgSize, 0)){
				close(s);			
				fclose(f);			
				logError(hostName, port, fileName,"TCP", -1 , "Unable to send ack");
	                	exit(EXIT_FAILURE);
			}
		}else{
			if(msgSize != sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in))){
				close(s);			
				fclose(f);			
				logError(hostName, port, fileName,"UDP", -1 , "Unable to send ack");
	                	exit(EXIT_FAILURE);
			}
		}
				
		//log received data 
		if(mode == TCP_MODE)
			logConnection(hostName, port, file, "TCP", datamsg.blockNumber, LOG_READ);
		else
			logConnection(hostName, port, file, "UDP", datamsg.blockNumber, LOG_READ);
	}

	close(s);
	fclose(f);

	if(mode == TCP_MODE)
		logConnection(hostName, port, file, "TCP", 0, LOG_END);
	else
		logConnection(hostName, port, file, "UDP", 0, LOG_END);
}

void TFTPclientWriteMode(ProtocolMode mode,char * hostName, char * file){


}

void logConnection(char * hostName, int port, char * fileName, char * protocol, int block, int mode){
	char toLog[LOG_MESSAGE_SIZE];
	char path[20];

	sprintf(path,"log/%d.txt",port);
	FILE*logFile = fopen(path,"a+");

	switch(mode){
		case LOG_START_READ:  
			sprintf(toLog,"\n[%s][HOST: %s][PROTOCOL: %s][PORT: %d][FILE: %s][STARTED READ]",  getDateAndTime(), hostName, protocol, port, fileName); 
		break;
		case LOG_START_WRITE: 
			sprintf(toLog,"\n[%s][HOST: %s][PROTOCOL: %s][PORT: %d][FILE: %s][STARTED WRITE]", getDateAndTime(), hostName, protocol, port, fileName); 
		break;
		case LOG_READ:        
			sprintf(toLog,"\n[%s][HOST: %s][PROTOCOL: %s][PORT: %d][FILE: %s][RECIVED: %d]",   getDateAndTime(), hostName, protocol, port, fileName, block); 
		break;
		case LOG_WRITE:       
			sprintf(toLog,"\n[%s][HOST: %s][PROTOCOL: %s][PORT: %d][FILE: %s][SEND: %d]",      getDateAndTime(), hostName, protocol, port, fileName, block); 
		break;
		case LOG_END:         
			sprintf(toLog,"\n[%s][HOST: %s][PROTOCOL: %s][PORT: %d][FILE: %s][SUCCED]",        getDateAndTime(), hostName, protocol, port, fileName); 
		break;
	}

	fprintf(stderr,"%s",toLog);
	fprintf(logFile,"%s",toLog);

	fclose(logFile);
}

void logError(char * hostName, int port, char * fileName, char * protocol, int errorCode, char * errormsg){
	char toLog[LOG_MESSAGE_SIZE];
	char error[ENUMERATION_SIZE];
	char path[CLIENT_FILE_PATH_SIZE];

	sprintf(path,"log/%d.txt",port);
	FILE*logFile = fopen(path,"a+");
    
	switch (errorCode) {
		case 0: strcpy(error,"UNKNOWN"); break;
		case 1: strcpy(error,"FILE_NOT_FOUND"); break;
		case 3: strcpy(error,"DISK_FULL"); break;
		case 4: strcpy(error,"ILEGAL_OPERATION"); break;
		case 6: strcpy(error,"FILE_ALREADY_EXISTS"); break;
	}

	sprintf(toLog,sprintf(toLog,"\n[%s][ERROR][HOST: %s][PROTOCOL: %s][PORT: %d][FILE: %s][CODE: %s][MSG: %s]"getDateAndTime(), hostName, protocol, port, fileName, error, errormsg);
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
