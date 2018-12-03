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
        fprintf(stderr, "\n[%s:%d:%s][%d]%s", __FILE__, __LINE__, __FUNCTION__,wrongValue,errorMsg); \
        exit(0);                                                                        \
    }		    	                                                                    \
}while(0)



#define LOG_START_READ 0 
#define LOG_START_WRITE 1 
#define LOG_READ 2
#define LOG_WRITE 3 
#define LOG_END 4

void logError(char * hostName, int port, int errorCode, char * errormsg);
void logc(char * hostName, int port, char * fileName, int block, int mode);

void tcpClient(bool isReadMode, char * hostName, char * file);
void udpClient(bool isReadMode, char * hostName, char * file);

int main(int argc, char * argv[]){
	//Register handlers for signals

	//Check the args are correct
	if( argc != 5
		|| !(!strcmp(argv[2],TCP_ARG) || !strcmp(argv[2],UDP_ARG))
		|| !(!strcmp(argv[3],READ_ARG)|| !strcmp(argv[3],WRITE_ARG))
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


void SIGALRMHandler(int ss){
  fprintf(stderr, "\nthe timeout passed");
  exit(EXIT_SUCCESS);
}

//=================================================================================================

void redefineSignal(int signal, void(*function)(int)){
  struct sigaction ss;
  memset(&ss,0,sizeof(ss));

  ss.sa_handler=function;
  ss.sa_flags=0;

  EXIT_ON_WRONG_VALUE(-1,"signal error",sigfillset(&ss.sa_mask));
  EXIT_ON_WRONG_VALUE(-1,"signal error",sigaction(signal,&ss,NULL));
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
	char tag[1000];
	char eTag[100];

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
	logc(hostName, port, file, 0, isReadMode? LOG_START_READ : LOG_START_WRITE);

	//send request to start protocol
	msgSize = fillBufferWithReadMsg(isReadMode,file, buffer);
	EXIT_ON_WRONG_VALUE(TRUE,"Error on sending read/write request",(send(s, buffer, msgSize, 0) != msgSize));

	//check if file exists
	bool fileExists = (access( file, F_OK ) != -1) ? TRUE : FALSE;

	//bifurcate into read or write
	if(isReadMode){

		//open the file to write
		char destionationFile[30];
		sprintf(destionationFile,"log/%s",file);
	    if(NULL == (f = fopen(destionationFile,"wb"))){
	        //if error send error msg 
        	sprintf(tag,"client couln't open %s" ,file);
	        msgSize = fillBufferWithErrMsg(UNKNOWN,tag, buffer);
	        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(send(s, buffer, msgSize, 0) != msgSize));
	        logError(hostName,port,UNKNOWN, tag);
	        exit(EXIT_FAILURE);
	    }

		while(!endSesion){
			//recive first one block
			msgSize = reciveMsg(s,buffer);
			if(msgSize < 0){
				msgSize = fillBufferWithErrMsg(UNKNOWN,"Error on reciving block", buffer);
				EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(send(s, buffer, msgSize, 0) != msgSize));
				logError(hostName,port,UNKNOWN, "Error on reciving block");
				exit(EXIT_FAILURE);
			}

			//act according to type
			switch(getMessageTypeWithBuffer(buffer)){
				case DATA_TYPE:
					datamsg = fillDataWithBuffer(msgSize,buffer);

					//check if block number is the correct one
					if(datamsg.blockNumber != blockNumber ){
						msgSize = fillBufferWithErrMsg(UNKNOWN,"recived blockNumber doesn't match with current blockNumber", buffer);
					    EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
					    logError(hostName,port,UNKNOWN, "recived blockNumber doesn't match with current blockNumber");
					    exit(EXIT_FAILURE);
					}
					//write the send data
					writeResult = fwrite(datamsg.data,1,DATA_SIZE(msgSize),f);
					if(-1 == writeResult){
						sprintf(tag,"error writing the file %s" ,file);
						msgSize = fillBufferWithErrMsg(DISK_FULL,tag, buffer);
					    EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
					    logError(hostName,port,DISK_FULL, tag);
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
					sprintf(tag,"received error when waiting data %d: %s",errmsg.errorCode, errmsg.errorMsg); 
		            logError(hostName,port,errmsg.errorCode, tag);
		            exit(EXIT_FAILURE);
				break;
				default:
					sprintf(tag,"unrecognized operation in current context %d" ,getMessageTypeWithBuffer(buffer));
					msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,tag, buffer);
	                EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
	                logError(hostName,port,ILLEGAL_OPERATION, tag);
	                exit(EXIT_FAILURE);
				break; 
			}

			//send ack
			msgSize = fillBufferWithAckMsg(datamsg.blockNumber, buffer);
			EXIT_ON_WRONG_VALUE(TRUE,"Error on sending ack for block",(send(s, buffer, msgSize, 0) != msgSize));
			//log received data 
			logc(hostName, port, file, datamsg.blockNumber, LOG_READ);
		}

	}else{

      //open the request file
      if(NULL == (f = fopen(file,"rb"))){
        //if error send error msg 
        sprintf(tag,"client couln't found %s" ,file);
        msgSize = fillBufferWithErrMsg(UNKNOWN,tag, buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(send(s, buffer, msgSize, 0) != msgSize));
        logError(hostName,port,UNKNOWN, tag);
        exit(EXIT_FAILURE);
      }

      while(!endSesion){
         //wait for ack 
          msgSize = reciveMsg(s,buffer);
          if(msgSize < 0){
            msgSize = fillBufferWithErrMsg(UNKNOWN,"Error on receiving ack", buffer);
            EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(send(s, buffer, msgSize, 0) != msgSize));
            logError(hostName,port,UNKNOWN, "Error on receiving ack");
            exit(EXIT_FAILURE);
          }

          switch(getMessageTypeWithBuffer(buffer)){
            case ACK_TYPE: 
              ackmsg = fillAckWithBuffer(buffer);
              //check if ack its correct; if not an error is send
              if( blockNumber != ackmsg.blockNumber ){
                msgSize = fillBufferWithErrMsg(UNKNOWN,"recived blockNumber doesn't match with current blockNumber", buffer);
                EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
                logError(hostName,port,UNKNOWN, "recived blockNumber doesn't match with current blockNumber");
                exit(EXIT_FAILURE);
              }
              blockNumber += 1;
            break;
            case ERR_TYPE: 
              errmsg = fillErrWithBuffer(buffer);
              sprintf(tag,"received error when waiting ack %d: %s",errmsg.errorCode, errmsg.errorMsg); 
              logError(hostName,port,errmsg.errorCode, tag);
              exit(EXIT_FAILURE);
            break;
            default:
              sprintf(tag,"unrecognized operation in current context %d" ,getMessageTypeWithBuffer(buffer));
              msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,tag, buffer);
              EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
              logError(hostName,port,ILLEGAL_OPERATION, tag);
              exit(EXIT_FAILURE);
            break; 
          }

        //send block 
        memset(dataBuffer,0,MSG_DATA_SIZE);
        readSize = fread(dataBuffer, 1, MSG_DATA_SIZE, f);
        if(-1 == readSize){
          sprintf(tag,"error reading the file %s" ,file);
          msgSize = fillBufferWithErrMsg(UNKNOWN,tag, buffer);
          EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
          logError(hostName,port,UNKNOWN, tag);
          exit(EXIT_FAILURE);
        }

        msgSize = fillBufferWithDataMsg(blockNumber,dataBuffer,readSize,buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending data block",(send(s, buffer, msgSize, 0) != msgSize));

        logc(hostName,port, file, blockNumber,LOG_WRITE);

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

#define TIMEOUT 5
int reciveUdpMsg(int s, char * buffer, struct sockaddr_in* serverData){
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

    int size = sizeof(struct sockaddr_in);
    alarm(TIMEOUT);
    j = recvfrom(s, buffer, TAM_BUFFER, 0,(struct sockaddr*)serverData,&size);
    if(j< TAM_BUFFER)
        buffer[j] = '\0';
    return j;
}


void udpClient(bool isReadMode, char * hostName, char * file){
	char tag[1000];
	char eTag[100];

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



  int i, errcode;
    int s;        /* socket descriptor */
    long timevar;                       /* contains time returned by time() */
    struct sockaddr_in myaddr_in; /* for local socket address */
    struct sockaddr_in servaddr_in; /* for server socket address */
    struct in_addr reqaddr;   /* for returned internet address */
    int addrlen, n_retry;
    struct sigaction vec;
    struct addrinfo hints, *res;



    /* Create the socket. */
  s = socket (AF_INET, SOCK_DGRAM, 0);
  if (s == -1) {
    fprintf(stderr, ": unable to create socket\n");
    exit(1);
  }
  


    /* clear out address structures */
  memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
  memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
  
      /* Bind socket to some local address so that the
     * server can send the reply back.  A port number
     * of zero will be used so that the system will
     * assign any available port number.  An address
     * of INADDR_ANY will be used so we do not have to
     * look up the internet address of the local host.
     */


  myaddr_in.sin_family = AF_INET;
  myaddr_in.sin_port = 0;
  myaddr_in.sin_addr.s_addr = INADDR_ANY;
  if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
    fprintf(stderr, ": unable to bind socket\n");
    exit(1);
     }
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
            fprintf(stderr, ": unable to read socket address\n");
            exit(1);
    }


  

  //log the connection
  port = ntohs(myaddr_in.sin_port);
  logc(hostName, port, file, 0, isReadMode? LOG_START_READ : LOG_START_WRITE);



  /* Set up the server address. */
  servaddr_in.sin_family = AF_INET;
    /* Get the host information for the server's hostname that the
     * user passed in.
     */
      memset (&hints, 0, sizeof (hints));
      hints.ai_family = AF_INET;
   /* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
    errcode = getaddrinfo (hostName, NULL, &hints, &res); 
    if (errcode != 0){
      /* Name was not found.  Return a
       * special value signifying the error. */
    fprintf(stderr, ": No es posible resolver la IP de\n");
    exit(1);
      }
    else {
      /* Copy address of host */
    servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
   }
     freeaddrinfo(res);
     /* puerto del servidor en orden de red*/
   servaddr_in.sin_port = htons(PORT);

   /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
    redefineSignal(SIGALRM,SIGALRMHandler);



	//send request to start protocol
	msgSize = fillBufferWithReadMsg(isReadMode,file, buffer);
	EXIT_ON_WRONG_VALUE(TRUE,"Error on sending read/write request",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) != msgSize));

	//check if file exists
	bool fileExists = (access( file, F_OK ) != -1) ? TRUE : FALSE;

	//bifurcate into read or write
	if(isReadMode){

		//open the file to write
		char destionationFile[30];
		sprintf(destionationFile,"log/%s",file);
	    if(NULL == (f = fopen(destionationFile,"wb"))){
	        //if error send error msg 
        	sprintf(tag,"client couln't open %s" ,file);
	        msgSize = fillBufferWithErrMsg(UNKNOWN,tag, buffer);
	        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) != msgSize));
	        logError(hostName,port,UNKNOWN, tag);
	        exit(EXIT_FAILURE);
	    }

		while(!endSesion){
			//recive first one block
			msgSize = reciveUdpMsg(s,buffer,&servaddr_in);
			if(msgSize < 0){
				msgSize = fillBufferWithErrMsg(UNKNOWN,"Error on reciving block", buffer);
				EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) != msgSize));
				logError(hostName,port,UNKNOWN, "Error on reciving block");
				exit(EXIT_FAILURE);
			}

			//act according to type
			switch(getMessageTypeWithBuffer(buffer)){
				case DATA_TYPE:
					datamsg = fillDataWithBuffer(msgSize,buffer);

					//check if block number is the correct one
					if(datamsg.blockNumber != blockNumber ){
            fprintf(stderr, "\n%d %d",datamsg.blockNumber,blockNumber);
						msgSize = fillBufferWithErrMsg(UNKNOWN,"recived blockNumber doesn't match with current blockNumber", buffer);
					    EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) != msgSize));
					    logError(hostName,port,UNKNOWN, "recived blockNumber doesn't match with current blockNumber");
					    exit(EXIT_FAILURE);
					}
					//write the send data
					writeResult = fwrite(datamsg.data,1,DATA_SIZE(msgSize),f);
					if(-1 == writeResult){
						sprintf(tag,"error writing the file %s" ,file);
						msgSize = fillBufferWithErrMsg(DISK_FULL,tag, buffer);
					    EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) != msgSize));
					    logError(hostName,port,DISK_FULL, tag);
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
					sprintf(tag,"received error when waiting data %d: %s",errmsg.errorCode, errmsg.errorMsg); 
		            logError(hostName,port,errmsg.errorCode, tag);
		            exit(EXIT_FAILURE);
				break;
				default:
					sprintf(tag,"unrecognized operation in current context %d" ,getMessageTypeWithBuffer(buffer));
					msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,tag, buffer);
	                EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) != msgSize));
	                logError(hostName,port,ILLEGAL_OPERATION, tag);
	                exit(EXIT_FAILURE);
				break; 
			}

			//send ack
			msgSize = fillBufferWithAckMsg(datamsg.blockNumber, buffer);
			EXIT_ON_WRONG_VALUE(TRUE,"Error on sending ack for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) != msgSize));
			//log received data 
			logc(hostName, port, file, datamsg.blockNumber, LOG_READ);
		}

	}else{

      //open the request file
      if(NULL == (f = fopen(file,"rb"))){
        //if error send error msg 
        sprintf(tag,"client couln't found %s" ,file);
        msgSize = fillBufferWithErrMsg(UNKNOWN,tag, buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) != msgSize));
        logError(hostName,port,UNKNOWN, tag);
        exit(EXIT_FAILURE);
      }

      while(!endSesion){
         //wait for ack 
          msgSize = reciveUdpMsg(s,buffer,&servaddr_in);
          if(msgSize < 0){
            msgSize = fillBufferWithErrMsg(UNKNOWN,"Error on receiving ack", buffer);
            EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) != msgSize));
            logError(hostName,port,UNKNOWN, "Error on receiving ack");
            exit(EXIT_FAILURE);
          }

          switch(getMessageTypeWithBuffer(buffer)){
            case ACK_TYPE: 
              ackmsg = fillAckWithBuffer(buffer);
              //check if ack its correct; if not an error is send
              if( blockNumber != ackmsg.blockNumber ){
                msgSize = fillBufferWithErrMsg(UNKNOWN,"recived blockNumber doesn't match with current blockNumber", buffer);
                EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) != msgSize));
                logError(hostName,port,UNKNOWN, "recived blockNumber doesn't match with current blockNumber");
                exit(EXIT_FAILURE);
              }
              blockNumber += 1;
            break;
            case ERR_TYPE: 
              errmsg = fillErrWithBuffer(buffer);
              sprintf(tag,"received error when waiting ack %d: %s",errmsg.errorCode, errmsg.errorMsg); 
              logError(hostName,port,errmsg.errorCode, tag);
              exit(EXIT_FAILURE);
            break;
            default:
              sprintf(tag,"unrecognized operation in current context %d" ,getMessageTypeWithBuffer(buffer));
              msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,tag, buffer);
              EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) != msgSize));
              logError(hostName,port,ILLEGAL_OPERATION, tag);
              exit(EXIT_FAILURE);
            break; 
          }

        //send block 
        memset(dataBuffer,0,MSG_DATA_SIZE);
        readSize = fread(dataBuffer, 1, MSG_DATA_SIZE, f);
        if(-1 == readSize){
          sprintf(tag,"error reading the file %s" ,file);
          msgSize = fillBufferWithErrMsg(UNKNOWN,tag, buffer);
          EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) != msgSize));
          logError(hostName,port,UNKNOWN, tag);
          exit(EXIT_FAILURE);
        }

        msgSize = fillBufferWithDataMsg(blockNumber,dataBuffer,readSize,buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending data block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&servaddr_in,sizeof(struct sockaddr_in)) != msgSize));

        logc(hostName,port, file, blockNumber,LOG_WRITE);

        //check if the file has ended
        if(feof(f))
          endSesion = TRUE;
      }
	}


	fclose(f);
	logc(hostName, port, file, 0, LOG_END);

	close(s);}

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

  sprintf(path,"log/%d.txt",port);
  FILE*logFile = fopen(path,"a+");

  switch(mode){
	case LOG_START_READ: sprintf(toLog,"\n[%s][Host: %s][Port:%d][File %s][CONNECTION STARTED][READ MODE]",getDateAndTime(),hostName, port,fileName); break;
    case LOG_START_WRITE: sprintf(toLog,"\n[%s][Host: %s][Port:%d][File %s][CONNECTION STARTED][WRITE MODE]",getDateAndTime(),hostName, port,fileName); break;
  	case LOG_READ: sprintf(toLog,"\n[%s][Host: %s][Port:%d][File %s][Recived block:%d]",getDateAndTime(),hostName, port,fileName,block); break;
  	case LOG_WRITE: sprintf(toLog,"\n[%s][Host: %s][Port:%d][File %s][Send block:%d]",getDateAndTime(),hostName, port,fileName,block); break;
  	case LOG_END: sprintf(toLog,"\n[%s][Host: %s][Port:%d][File %s][SUCCED]",getDateAndTime(),hostName, port,fileName); break;
  }

  fprintf(stderr,"%s",toLog);
  fprintf(logFile, "%s",toLog);

  fclose(logFile);
}

void logError(char * hostName, int port, int errorCode, char * errormsg){
	char toLog[LOG_MESSAGE_SIZE];
    char error[ENUMERATION_SIZE];
	char path[CLIENT_FILE_PATH_SIZE];

    sprintf(path,"log/%d.txt",port);
    FILE*logFile = fopen(path,"a+");
    
    switch (errorCode) {
        case 0:
            strcpy(error,"UNKNOWN");
            break;
        case 1:
            strcpy(error,"FILE_NOT_FOUND");
            break;
        case 3:
            strcpy(error,"DISK_FULL");
            break;
        case 4:
            strcpy(error,"ILEGAL_OPERATION");
            break;
        case 6:
            strcpy(error,"FILE_ALREADY_EXISTS");
            break;
    }

	sprintf(toLog,"\n[%s][Host: %s][Port:%d][ERROR: %s %s]",getDateAndTime(),hostName,port,error, errormsg);
	fprintf(stderr,"%s",toLog);
	fprintf(logFile, "%s",toLog);

	fclose(logFile);
}
