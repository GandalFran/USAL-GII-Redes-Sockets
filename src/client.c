/*

** Fichero: cliente.c
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

#define EXIT_ON_WRONG_VALUE(wrongValue, errorMsg, returnValue)                          \
do{                                              		    	                              \
    if((returnValue) == (wrongValue)){		    	                                        \
        char errorTag[200];                                                             \
        fprintf(stderr, "\n[%s:%d:%s]%s", __FILE__, __LINE__, __FUNCTION__,errorMsg);   \
        exit(EXIT_SUCCESS);		    	                                                    \
        exit(0);                                                                        \
    }		    	                                                                          \
}while(0)

#define LOG_START 0 
#define LOG_NORMAL 1 
#define LOG_END 2

void logError(char * hostName, int port, int errorCode, char * errormsg);
void logc(char * hostName, int port, char * fileName, int block, int mode);

void tcpClient(bool isReadMode, char * hostName, char * file);
void udpClient(bool isReadMode, char * hostName, char * file);

int main(int argc, char * argv[]){
	//Register handlers for signals

	//Check the args are correct
	if( argc != 5
		|| (!strcmp(argv[2],TCP_ARG) && !strcmp(argv[2],UDP_ARG))
		|| (!strcmp(argv[3],READ_ARG) && !strcmp(argv[3],WRITE_ARG))
	){
		fprintf(stderr,"%s",USAGE_ERROR_MSG);
	}

	if(!strcmp(argv[2],TCP_ARG)){
		tcpClient(!strcmp(READ_ARG,argv[3]),argv[1],argv[4]);
	}else{
		udpClient(!strcmp(READ_ARG,argv[3]),argv[1],argv[4]);
	}

	return 0;
}

//========================================================================================================================

int reciveMsg(int s, char * buffer){
  int i=0,j;
  memset(buffer,0,sizeof(buffer));
/*
    while (i = recv(s, buffer, TAM_BUFFER, 0) && i<sizeof(rwMsg)) {
      EXIT_ON_WRONG_VALUE(-1,"error reading msg", i);
      logIssue("enteringLoop2");
      while (i < TAM_BUFFER ) {
      logIssue("into loop 2");
        j = recv(s, &buffer[i], TAM_BUFFER-i, 0);
        logIssue("loop2.2");
        EXIT_ON_WRONG_VALUE(-1,"error reading msg", j);
        i += j;
      }
      logIssue("loop2ended");
    }
    logIssue("loop3");
*/

  recv(s, buffer, TAM_BUFFER, 0);
}


/*
 *			C L I E N T C P
 *
 *	This is an example program that demonstrates the use of
 *	stream sockets as an IPC mechanism.  This contains the client,
 *	and is intended to operate in conjunction with the server
 *	program.  Together, these two programs
 *	demonstrate many of the features of sockets, as well as good
 *	conventions for using these features.
 *
 *
 */

void tcpClient(bool isReadMode, char * hostName, char * file){
    int s;				/* connected socket descriptor */
    int numberOfmessages = 0;
    struct addrinfo hints, *res;
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in servaddr_in;	/* for server socket address */
	int addrlen, i, j, errcode;
    /* This example uses BUFFER_SIZE byte messages. */
	char buffer[TAM_BUFFER];

	/* Create the socket. */
	EXIT_ON_WRONG_VALUE(-1,"unable to create socket",s = socket (AF_INET, SOCK_STREAM, 0));

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
	errcode = getaddrinfo (hostName, NULL, &hints, &res);
	EXIT_ON_WRONG_VALUE(1,"Not possible to solve IP",errcode != 0);

	/* Copy address of host */
	servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;

 	freeaddrinfo(res);

  	/* puerto del servidor en orden de red*/
	servaddr_in.sin_port = htons(PORT);

	/* Try to connect to the remote server at the address
	 *  which was just built into peeraddr.
	 */
	EXIT_ON_WRONG_VALUE(-1, " unable to connect to remote", connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)));
	/* Since the connect call assigns a free address
	 * to the local end of this connection, let's use
	 * getsockname to see what it assigned.  Note that
	 * addrlen needs to be passed in as a pointer,
	 * because getsockname returns the actual length
	 * of the address.
	 */
	addrlen = sizeof(struct sockaddr_in);
	//returns the information of our socket
	EXIT_ON_WRONG_VALUE(-1,"unable to read socket address", getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen));


//======================================================================================================================
	rwMsg requestMsg;
	dataMsg datamsg;
	ackMsg ackmsg;
	errMsg errmsg;
	headers msgType;
	bool end;
	FILE * f = NULL;
	long msgSize;

	//log the connection
	logc(hostName, ntohs(myaddr_in.sin_port), file, 0, LOG_START);

	//send request to start protocol
	fillBufferWithReadMsg(isReadMode,file, buffer);
	EXIT_ON_WRONG_VALUE(TRUE,"Error on sending read/write request",(send(s, buffer, TAM_BUFFER, 0) != TAM_BUFFER));

	//now is bifurcated in readmode and writemode + the file is opened
	char destionationFile[30];
	sprintf(destionationFile,"CLIENT%s",file);
	f = fopen(file,"wb+");

	end = FALSE;
	if(isReadMode){
		while(!end){
			msgSize = reciveMsg(s,buffer);
			msgType = getMessageTypeWithBuffer(buffer);
			switch(msgType){
				case DATA: 
					datamsg = fillDataWithBuffer(buffer);
					fwrite(datamsg.data,sizeof(char),MSG_DATA_SIZE,f);
				break;
				case ERR: 
					errmsg = fillErrWithBuffer(buffer);
					logError("desde la 200", ntohs(myaddr_in.sin_port), errmsg.errorCode, errmsg.errorMsg);
					exit(EXIT_FAILURE);
				break;
				default:
					fillBufferWithErrMsg(ILLEGAL_OPERATION,"ILLEGAL_OPERATION", buffer);
					EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, TAM_BUFFER, 0) != TAM_BUFFER));
					logError(hostName, ntohs(myaddr_in.sin_port), /*errmsg.errorCode*/msgType, "ILLEGAL_OPERATION");
					exit(EXIT_FAILURE);
				break; 
			}

			//send ack
			fillBufferWithAckMsg(datamsg.blockNumber, buffer);
			EXIT_ON_WRONG_VALUE(TRUE,"Error on sending ack for block",(send(s, buffer, TAM_BUFFER, 0) != TAM_BUFFER));

			if(msgSize != TAM_BUFFER*sizeof(char))
				end = TRUE;

			//log received data 
			logc(hostName, ntohs(myaddr_in.sin_port), file, datamsg.blockNumber, LOG_NORMAL);
		}

	}else{


	}


	fclose(f);
	logc(hostName, ntohs(myaddr_in.sin_port), file, 0, LOG_END);

	/* Now, shutdown the connection for further sends.
	 * This will cause the server to receive an end-of-file
	 * condition after it has received all the requests that
	 * have just been sent, indicating that we will not be
	 * sending any further requests.
	 */
	EXIT_ON_WRONG_VALUE(-1,"unable to shutdown socket",shutdown(s, 1));

	close(s);

}


void udpClient(bool isReadMode, char * hostName, char * file){


}

//========================================================================================================================

#define CLIENT_FILE_PATH_SIZE 20

const char * getDateAndTime(){
	static char timeString[TIME_STRING_SIZE];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(timeString,"%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	return timeString;
}

void logc(char * hostName, int port, char * fileName, int block, int mode){
  char toLog[LOG_MESSAGE_SIZE];
  char path[CLIENT_FILE_PATH_SIZE];

  sprintf(path,"%d.txt",port);
  FILE*logFile = fopen(path,"a+");

  switch(mode){
	case LOG_START: sprintf(toLog,"\n[%s][Host: %s][Port:%d][File %10s][CONNECTION STARTED]",getDateAndTime(),hostName, port,fileName); break;
  	case LOG_NORMAL: sprintf(toLog,"\n[%s][Host: %s][Port:%d][File %10s][Send block:%d]",getDateAndTime(),hostName, port,fileName,block); break;
  	case LOG_END: sprintf(toLog,"\n[%s][Host: %s][Port:%d][File %10s][SUCCED]",getDateAndTime(),hostName, port,fileName); break;
  }

  fprintf(stderr,"%s",toLog);
  fprintf(logFile, "%s",toLog);

  fclose(logFile);
}

void logError(char * hostName, int port, int errorCode, char * errormsg){
	char toLog[LOG_MESSAGE_SIZE];
	char path[CLIENT_FILE_PATH_SIZE];

    sprintf(path,"%d.txt",port);
    FILE*logFile = fopen(path,"a+");

	sprintf(toLog,"\n[%s][Host: %s][Port:%d][ERROR: %d %s]",getDateAndTime(),hostName,port,errorCode, errormsg);
	fprintf(stderr,"%s",toLog);
	fprintf(logFile, "%s",toLog);

	fclose(logFile);
}
