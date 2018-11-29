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
do{                                              		    	                        \
    if((returnValue) == (wrongValue)){		    	                                    \
        char errorTag[200];                                                             \
        fprintf(stderr, "\n[%s:%d:%s]%s", __FILE__, __LINE__, __FUNCTION__,errorMsg);   \
        exit(EXIT_SUCCESS);		    	                                                \
        exit(0);                                                                        \
    }		    	                                                                    \
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
  	j = recv(s, buffer, TAM_BUFFER, 0);
  	if(j< TAM_BUFFER && j>0)
  		buffer[j] = '\0';
  	return j;
}


void tcpClient(bool isReadMode, char * hostName, char * file){
    int s, addrlen, errcode;				
    struct addrinfo hints, *res;
    struct sockaddr_in myaddr_in;	    
    struct sockaddr_in servaddr_in;	

    bool endSesion;

    int port;

	rwMsg requestMsg;
	dataMsg datamsg;
	ackMsg ackmsg;
	errMsg errmsg;
	headers msgType;

	FILE * f = NULL;
	int blockNumber = 0;
	int msgSize, writeResult, readSize;
  	char dataBuffer[MSG_DATA_SIZE];

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
	servaddr_in.sin_port = htons(PORT);
	EXIT_ON_WRONG_VALUE(-1, " unable to connect to remote", connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)));

	//returns the information of our socket
	addrlen = sizeof(struct sockaddr_in);
	EXIT_ON_WRONG_VALUE(-1,"unable to read socket address", getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen));

	//log the connection
	port = ntohs(myaddr_in.sin_port);
	logc(hostName, port, file, 0, LOG_START);

	//send request to start protocol
	msgSize = fillBufferWithReadMsg(isReadMode,file, buffer);
	EXIT_ON_WRONG_VALUE(TRUE,"Error on sending read/write request",(send(s, buffer, msgSize, 0) != msgSize));

	//check if file exists
	bool fileExists = (access( file, F_OK ) != -1) ? TRUE : FALSE;

	//bifurcate into read or write
	if(isReadMode){

		//open the file to write
		char destionationFile[30];
		sprintf(destionationFile,"CLIENT.txt");
	    if(NULL == (f = fopen(destionationFile,"wb+"))){
	        //if error send error msg 
	        msgSize = fillBufferWithErrMsg(UNKNOWN,"UNKNOWN", buffer);
	        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(send(s, buffer, msgSize, 0) != msgSize));
	        logError(hostName,port,UNKNOWN, file);
	        exit(EXIT_FAILURE);
	    }

		while(!endSesion){
			//recive first one block
			msgSize = reciveMsg(s,buffer);
			if(msgSize < 0){
				msgSize = fillBufferWithErrMsg(UNKNOWN,"UNKNOWN", buffer);
				EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(send(s, buffer, msgSize, 0) != msgSize));
				logError(hostName,port,UNKNOWN, "UNKNOWNa");
				exit(EXIT_FAILURE);
			}

			//act according to type
			switch(getMessageTypeWithBuffer(buffer)){
				case DATA_TYPE:

					datamsg = fillDataWithBuffer(msgSize,buffer);

					//check if block number is the correct one
					if(datamsg.blockNumber != blockNumber ){
						msgSize = fillBufferWithErrMsg(UNKNOWN,"UNKNOWN", buffer);
					    EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
					    logError(hostName,port,UNKNOWN, "UNKNOWNa");
					    exit(EXIT_FAILURE);
					}
					//write the send data
					writeResult = fwrite(datamsg.data,sizeof(char),strlen(datamsg.data),f);
					if(-1 == writeResult){
						msgSize = fillBufferWithErrMsg(DISK_FULL,"DISK_FULL", buffer);
					    EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
					    logError(hostName,port,DISK_FULL, "DISK_FULL");
					    exit(EXIT_FAILURE);
					}
					//check if the block is the last -- size less than 512 bytes
					if(sizeof(datamsg.data) < MSG_DATA_SIZE)
						endSesion = TRUE;
					//increment block number 
					blockNumber += 1;
				break;
				case ERR_TYPE: 
					errmsg = fillErrWithBuffer(buffer);
		            logError(hostName,port,errmsg.errorCode, errmsg.errorMsg);
		            exit(EXIT_FAILURE);
				break;
				default:
					msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,"ILLEGAL_OPERATION", buffer);
	                EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
	                logError(hostName,port,getMessageTypeWithBuffer(buffer), &buffer[4]);
	                exit(EXIT_FAILURE);
				break; 
			}

			//send ack
			msgSize = fillBufferWithAckMsg(datamsg.blockNumber, buffer);
			EXIT_ON_WRONG_VALUE(TRUE,"Error on sending ack for block",(send(s, buffer, msgSize, 0) != msgSize));
			//log received data 
			logc(hostName, port, file, datamsg.blockNumber, LOG_NORMAL);
		}

	}else{

      //open the request file
      if(NULL == (f = fopen(file,"rb"))){
        //if error send error msg 
        msgSize = fillBufferWithErrMsg(UNKNOWN,"UNKNOWN", buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(send(s, buffer, msgSize, 0) != msgSize));
        logError(hostName,port,UNKNOWN, file);
        exit(EXIT_FAILURE);
      }

      while(!endSesion){
         //wait for ack 
          msgSize = reciveMsg(s,buffer);
          if(msgSize < 0){
            msgSize = fillBufferWithErrMsg(UNKNOWN,"UNKNOWN", buffer);
            EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(send(s, buffer, msgSize, 0) != msgSize));
            logError(hostName,port,UNKNOWN, "UNKNOWNa");
            exit(EXIT_FAILURE);
          }

          switch(getMessageTypeWithBuffer(buffer)){
            case ACK_TYPE: 
              ackmsg = fillAckWithBuffer(buffer);
              //check if ack its correct; if not an error is send
              if( blockNumber != ackmsg.blockNumber ){
                msgSize = fillBufferWithErrMsg(UNKNOWN,"UNKNOWN", buffer);
                EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
                logError(hostName,port,UNKNOWN, "line 240");
                exit(EXIT_FAILURE);
              }
              blockNumber += 1;
            break;
            case ERR_TYPE: 
              errmsg = fillErrWithBuffer(buffer);
              logError(hostName,port,errmsg.errorCode, errmsg.errorMsg);
              exit(EXIT_FAILURE);
            break;
            default:
              //logError(ILLEGAL_OPERATION, "ILLEGAL_OPERATION");
              //CAMBIAR POR EL DE ARRIBA AL TERMINAR
              logError(hostName,port,buffer[1], "ILLEGAL_OPERATION");
              msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,"ILLEGAL_OPERATION", buffer);
              EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
              exit(EXIT_FAILURE);
            break; 
          }

        //send block 
        memset(dataBuffer,0,MSG_DATA_SIZE);
        readSize = fread(dataBuffer, sizeof(char), MSG_DATA_SIZE, f);
        if(-1 == readSize){
          msgSize = fillBufferWithErrMsg(UNKNOWN,"UNKNOWN", buffer);
          EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
          logError(hostName,port,UNKNOWN, "line 267");
          exit(EXIT_FAILURE);
        }

        msgSize = fillBufferWithDataMsg(blockNumber,dataBuffer,readSize,buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending data block",(send(s, buffer, msgSize, 0) != msgSize));

        logc(hostName,port, file, blockNumber,LOG_NORMAL);

        //check if the file has ended
        if(feof(f))
          endSesion = TRUE;
      }
	}


	fclose(f);
	logc(hostName, port, file, 0, LOG_END);

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
