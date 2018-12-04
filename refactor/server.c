/*

** Fichero: server.c
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

extern int errno;

//var to end the server loop
bool end = FALSE;

//signal handlers
void SIGTERMHandler(int);
void SIGALRMHandler(int);
//servers impl
void tcpServer(int s, struct sockaddr_in clientaddr_in);
void udpServer(int s, char * buffer, struct sockaddr_in clientaddr_in);
//others
void redefineSignal(int signal, void(*function)(int));

#define LOG_START_READ 0 
#define LOG_START_WRITE 1 
#define LOG_READ 2
#define LOG_WRITE 3 
#define LOG_END 4
#define MAX_ATTEMPS 5
void logError(int errorcode, char * errorMsg);
void logs(char * file, char * host, char * ip, char * protocol, int clientPort, int blockNumber, int mode);
void logIssue(char * asdf);

#define EXIT_ON_WRONG_VALUE(wrongValue, errorMsg, returnValue)                          \
do{                                              		    	                              \
    if((returnValue) == (wrongValue)){		    	                                        \
        char errorTag[200];                                                             \
        sprintf(errorTag, "\n[%s:%d:%s]%s", __FILE__, __LINE__, __FUNCTION__,errorMsg); \
        logError(UNKNOWN,errorTag);		    	                                            \
        raise(SIGTERM);		    	                                                        \
        exit(0);                                                                        \
    }		    	                                                                          \
}while(0)

int main(int argc, char * argv[]){
  //Block all signals and redefine sigterm and sigalarm
    sigset_t signalSet;

    redefineSignal(SIGTERM,SIGTERMHandler);
    redefineSignal(SIGALRM,SIGALRMHandler);

    EXIT_ON_WRONG_VALUE(-1,"signal error",sigfillset(&signalSet));
    EXIT_ON_WRONG_VALUE(-1,"signal error",sigdelset(&signalSet,SIGTERM));
    EXIT_ON_WRONG_VALUE(-1,"signal error",sigdelset(&signalSet,SIGALRM));
    EXIT_ON_WRONG_VALUE(-1,"signal error",sigprocmask(SIG_SETMASK, &signalSet, NULL));

  //Server initialization -- from nines template
      int s_TCP, s_UDP;		/* connected socket descriptor */
      int ls_TCP;				/* listen socket descriptor */

      int cc;				    /* contains the number of bytes read */

      struct sockaddr_in myaddr_in;	/* for local socket address */
      struct sockaddr_in clientaddr_in;	/* for peer socket address */
  	  int addrlen;

      fd_set readmask;
      int numfds,s_mayor;

      char buffer[TAM_BUFFER];	/* buffer for packets to be read into */

      struct sigaction vec;

  	/* Create the listen socket. */
  
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
		 * portable.
		 */
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
		 * is the largest currently supported.
		 */
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
  		/* Now, all the initialization of the server is
  		 * complete, and any user errors will have already
  		 * been detected.  Now we can fork the daemon and
  		 * return to the user.  We need to do a setpgrp
  		 * so that the daemon will no longer be associated
  		 * with the user's control terminal.  This is done
  		 * before the fork, so that the child will not be
  		 * a process group leader.  Otherwise, if the child
  		 * were to open a terminal, it would become associated
  		 * with that terminal as its control terminal.  It is
  		 * always best for the parent to do the setpgrp.
  		 */
  	setpgrp();

    pid_t daemonPid;
    EXIT_ON_WRONG_VALUE(-1,"unable to fork daemon",daemonPid = fork());
    if(0 != daemonPid){
      /*If is father it exits*/
      exit(EXIT_SUCCESS);
    }
	  /* The child process (daemon) comes here. */

	  /* Close stdin and stderr so that they will not
	   * be kept open.  Stdout is assumed to have been
	   * redirected to some logging file, or /dev/null.
	   * From now on, the daemon will not report any
	   * error messages.  This daemon will loop forever,
	   * waiting for connections and forking a child
	   * server to handle each one.
	   */
	   fclose(stdin);
	   fclose(stderr);

	   while (!end) {
        /* Meter en el conjunto de sockets los sockets UDP y TCP */
        FD_ZERO(&readmask);
        FD_SET(ls_TCP, &readmask);
        FD_SET(s_UDP, &readmask);
        /*Seleccionar el descriptor del socket que ha cambiado. Deja una marca en
        el conjunto de sockets (readmask)
        */

        s_mayor = (ls_TCP > s_UDP) ? ls_TCP : s_UDP;

        numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL);
        EXIT_ON_WRONG_VALUE(EINTR,"Closing server because of signal recived in select",errno);

        /* Comprobamos si el socket seleccionado es el socket TCP */
        if (FD_ISSET(ls_TCP, &readmask)) {

            /* Note that addrlen is passed as a pointer
             * so that the accept call can return the
             * size of the returned address.
             */
            /* This call will block until a new
             * connection arrives.  Then, it will
             * return the address of the connecting
             * peer, and a new socket descriptor, s,
             * for that connection.
             */
              EXIT_ON_WRONG_VALUE(-1,"unable to accept tcp socket from parent",s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen));

              pid_t serverInstancePid;
              EXIT_ON_WRONG_VALUE(-1,"unable to fork tcp server instance",serverInstancePid = fork());
              if(0 == serverInstancePid){
                  close(ls_TCP); /* Close the listen socket inherited from the daemon. */
                  tcpServer(s_TCP, clientaddr_in);
                  exit(EXIT_SUCCESS);
              }else{
                /* Daemon process comes here. */
                    /* The daemon needs to remember
                     * to close the new accept socket
                     * after forking the child.  This
                     * prevents the daemon from running
                     * out of file descriptor space.  It
                     * also means that when the server
                     * closes the socket, that it will
                     * allow the socket to be destroyed
                     * since it will be the last close.
                     */
                  close(s_TCP);
              }
         }
          /* Comprobamos si el socket seleccionado es el socket UDP */
          if (FD_ISSET(s_UDP, &readmask)) {
              /* This call will block until a new
               * request arrives.  Then, it will
               * return the address of the client,
               * and a buffer containing its request.
               * TAM_BUFFER - 1 bytes are read so that
               * room is left at the end of the buffer
               * for a null character.
               */
              EXIT_ON_WRONG_VALUE(-1,"recvfrom error",cc = recvfrom(s_UDP, buffer, TAM_BUFFER - 1, 0, (struct sockaddr *)&clientaddr_in, &addrlen));
              /* Make sure the message received is
               * null terminated.
               */
              buffer[cc]='\0';

		//nines
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
EXIT_ON_WRONG_VALUE(-1,"unable to read socket address", getsockname(s, (struct sockaddr *)&myaddrin, &addrlen));

              pid_t serverInstancePid;
              EXIT_ON_WRONG_VALUE(-1,"unable to fork tcp server instance",serverInstancePid = fork());
              if(0 == serverInstancePid){
                  udpServer (s, buffer, clientaddr_in);
                  exit(EXIT_SUCCESS);
              }else{
		continue;
	      }
		
          }
}
     //close handlers
     close(s_UDP);
     close(ls_TCP);
     exit(EXIT_SUCCESS);
}

#define HOSTNAME_SIZE 100
#define HOSTIP_SIZE 100



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
    if(j< TAM_BUFFER)
        buffer[j] = '\0';
    return j;
}



void tcpServer(int s, struct sockaddr_in clientaddr_in){
  char tag[1000];

  int port;
  char hostIp[HOSTIP_SIZE];
  char hostName[HOSTNAME_SIZE];

  ackMsg ackmsg;
  errMsg errmsg;
  dataMsg datamsg;
  rwMsg requestmsg;

  char buffer[TAM_BUFFER];

  FILE * f = NULL;
  int blockNumber = 0;
  int writeResult = 0;
  char dataBuffer[MSG_DATA_SIZE];

  int msgSize, readSize;
  bool endSesion = FALSE;

  //obtain the host information to start communicacion with remote host
  if( getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),hostName,sizeof(hostName),NULL,0,0) )
    EXIT_ON_WRONG_VALUE(NULL,"error inet_ntop",inet_ntop(AF_INET,&clientaddr_in.sin_addr,hostName,sizeof(hostName)));

  port = ntohs(clientaddr_in.sin_port);
  strcpy(hostIp, inet_ntoa(clientaddr_in.sin_addr));

  //set opMode
  struct linger lngr;
  lngr.l_onoff = 1;
  lngr.l_linger= 1;
  EXIT_ON_WRONG_VALUE(-1,hostName,setsockopt(s,SOL_SOCKET,SO_LINGER,&lngr,sizeof(lngr)));

  //read client request to know if is required read or write
  msgSize = reciveMsg(s,buffer);
  if(msgSize < 0){
    msgSize = fillBufferWithErrMsg(UNKNOWN,"error on receiving client request", buffer);
    EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(send(s, buffer, msgSize, 0) != msgSize));
    logError(UNKNOWN, "error on receiving client request");
    exit(EXIT_FAILURE);
  }

  requestmsg = fillReadMsgWithBuffer(buffer);
  //if the msg isnt a request send error 
  if(READ_TYPE != requestmsg.header && WRITE_TYPE != requestmsg.header){
    msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,"error on client request header: isn't read or write", buffer);
    EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
    logError(ILLEGAL_OPERATION, "error on client request header: isn't read or write");
    exit(EXIT_FAILURE);
  }

  logs(NULL, hostName, hostIp, "TCP", port, 0,(requestmsg.header == READ_TYPE)? LOG_START_READ : LOG_START_WRITE);


  //check if file exists
  bool fileExists = (access( requestmsg.fileName, F_OK ) != -1) ? TRUE : FALSE;

  //obtain the request type and bifurcate 
  if(requestmsg.header == READ_TYPE){

      //send error if file not found
      if(!fileExists){
        sprintf(tag,"server couln't found %s" ,requestmsg.fileName);
        msgSize = fillBufferWithErrMsg(FILE_NOT_FOUND,tag, buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
        logError(FILE_NOT_FOUND, tag);
        exit(EXIT_FAILURE);
      }

      //open the request file
      if(NULL == (f = fopen(requestmsg.fileName,"rb"))){
        //if error send error msg 
        sprintf(tag,"server couln't found %s" ,requestmsg.fileName);
        msgSize = fillBufferWithErrMsg(UNKNOWN, tag, buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
        logError(UNKNOWN, tag);
        exit(EXIT_FAILURE);
      }

      while(!endSesion){
        //send block 
          memset(dataBuffer,0,MSG_DATA_SIZE);
          readSize = fread(dataBuffer, 1, MSG_DATA_SIZE, f);
          if(-1 == readSize){
            sprintf(tag,"error reading the file %s" ,requestmsg.fileName);
            msgSize = fillBufferWithErrMsg(UNKNOWN,tag, buffer);
            EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
            logError(UNKNOWN, tag);
            exit(EXIT_FAILURE);
          }

          msgSize = fillBufferWithDataMsg(blockNumber,dataBuffer,readSize,buffer);
          EXIT_ON_WRONG_VALUE(TRUE,"Error on sending data block",(send(s, buffer, msgSize, 0) != msgSize));

          //logs(requestmsg.fileName,hostName, hostIp, "TCP", port, blockNumber,LOG_READ);

        //wait for ack 
          msgSize = reciveMsg(s,buffer);
          if(msgSize < 0){
            msgSize = fillBufferWithErrMsg(UNKNOWN,"Error on receiving ack", buffer);
            EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(send(s, buffer, msgSize, 0) != msgSize));
            logError(UNKNOWN, "Error on receiving ack");
            exit(EXIT_FAILURE);
          }

          switch(getMessageTypeWithBuffer(buffer)){
            case ACK_TYPE: 
              ackmsg = fillAckWithBuffer(buffer);
              //check if ack its correct; if not an error is send
              if( blockNumber != ackmsg.blockNumber ){
                msgSize = fillBufferWithErrMsg(UNKNOWN,"recived blockNumber doesn't match with current blockNumber", buffer);
                EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
                logError(UNKNOWN, "recived blockNumber doesn't match with current blockNumber");
                exit(EXIT_FAILURE);
              }
              blockNumber += 1;
            break;
            case ERR_TYPE:
              errmsg = fillErrWithBuffer(buffer);
              sprintf(tag,"received error when waiting ack %d: %s",errmsg.errorCode, errmsg.errorMsg); 
              logError(errmsg.errorCode, tag);
              exit(EXIT_FAILURE);
            break;
            default:
              sprintf(tag,"unrecognized operation in current context %d" ,getMessageTypeWithBuffer(buffer));
              msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,tag, buffer);
              EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
              logError(ILLEGAL_OPERATION, tag);
              exit(EXIT_FAILURE);
            break; 
          }

        //check if the file has ended
        if(feof(f))
          endSesion = TRUE;
      }

  }else{
      //send error if file found
    /* if(fileExists){
        sprintf(tag,"the requested file alerady exists: %s" ,requestmsg.fileName);
        msgSize = fillBufferWithErrMsg(FILE_ALREADY_EXISTS,tag, buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error becuase file exists",(send(s, buffer, msgSize, 0) != msgSize));
        logError(FILE_ALREADY_EXISTS, tag);
        exit(EXIT_FAILURE);
      }*/

      //open the request file
      char destionationFile[200];
      sprintf(destionationFile,"log/%s",requestmsg.fileName);
      if(NULL == (f = fopen(destionationFile,"wb+"))){
        //if error send error msg
        sprintf(tag,"error opening the file %s" ,requestmsg.fileName);
        msgSize = fillBufferWithErrMsg(UNKNOWN,tag, buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
        logError(UNKNOWN, tag);
        exit(EXIT_FAILURE);
      }

      //send ack and increment block
      msgSize = fillBufferWithAckMsg(datamsg.blockNumber, buffer);
      EXIT_ON_WRONG_VALUE(TRUE,"Error on sending ack for block",(send(s, buffer, msgSize, 0) != msgSize));
      blockNumber += 1;
      
      while(!endSesion){
      //recive block
      msgSize = reciveMsg(s,buffer);
      if(msgSize < 0){
        msgSize = fillBufferWithErrMsg(UNKNOWN,"Error on reciving block", buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(send(s, buffer, msgSize, 0) != msgSize));
        logError(UNKNOWN, "Error on reciving block");
        exit(EXIT_FAILURE);
      }

      //act according to type
      switch(getMessageTypeWithBuffer(buffer)){
        case DATA_TYPE:

          datamsg = fillDataWithBuffer(msgSize,buffer);

          //check if block number is the correct one
          if(datamsg.blockNumber != blockNumber ){
            msgSize = fillBufferWithErrMsg(UNKNOWN,"UNKNOWN", buffer);
              EXIT_ON_WRONG_VALUE(TRUE,"recived blockNumber doesn't match with current blockNumber",(send(s, buffer, msgSize, 0) != msgSize));
              logError(UNKNOWN, "recived blockNumber doesn't match with current blockNumber");
              exit(EXIT_FAILURE);
          }
          //write the send data
          writeResult = fwrite(datamsg.data,1,DATA_SIZE(msgSize),f);
          if(-1 == writeResult){
            sprintf(tag,"error writing the file %s" ,requestmsg.fileName);
            msgSize = fillBufferWithErrMsg(DISK_FULL,tag, buffer);
              EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
              logError(DISK_FULL, tag);
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
            logError(errmsg.errorCode, tag);
            exit(EXIT_FAILURE);
        break;
        default:
              sprintf(tag,"unrecognized operation in current context %d" ,getMessageTypeWithBuffer(buffer));
              msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,tag, buffer);
              EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(send(s, buffer, msgSize, 0) != msgSize));
              logError(ILLEGAL_OPERATION, tag);
              exit(EXIT_FAILURE);
        break; 
      }

      //send ack
      msgSize = fillBufferWithAckMsg(datamsg.blockNumber, buffer);
      EXIT_ON_WRONG_VALUE(TRUE,"Error on sending ack for block",(send(s, buffer, msgSize, 0) != msgSize));

      //log received data 
      //logs(requestmsg.fileName,hostName, hostIp, "TCP", port, blockNumber, LOG_WRITE);
    }
  }

    /* Now, shutdown the connection for further sends.
    * This will cause the server to receive an end-of-file
    * condition after it has received all the requests that
    * have just been sent, indicating that we will not be
    * sending any further requests.
    */
    EXIT_ON_WRONG_VALUE(-1,"unable to shutdown socket",shutdown(s, 1));

  fclose(f);
  close(s);
  logs(requestmsg.fileName,hostName, hostIp, "TCP", port, 0, LOG_END);
}


int reciveUdpMsg(int s, char * buffer, struct sockaddr_in * addr){
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
    j = recvfrom(s, buffer, TAM_BUFFER, 0,(struct sockaddr*)addr,&size);
    if(j< TAM_BUFFER)
        buffer[j] = '\0';
    return j;
}


void udpServer(int s, char * buffer, struct sockaddr_in clientaddr_in){
   char tag[1000];

  int port;
  char hostIp[HOSTIP_SIZE];
  char hostName[HOSTNAME_SIZE];

  ackMsg ackmsg;
  errMsg errmsg;
  dataMsg datamsg;
  rwMsg requestmsg;

  FILE * f = NULL;
  int blockNumber = 0;
  int writeResult = 0;
  char dataBuffer[MSG_DATA_SIZE];

  int msgSize, readSize;
  bool endSesion = FALSE;

logIssue("STARTED UDP");


//PROTOCOLO EN SI
  requestmsg = fillReadMsgWithBuffer(buffer);
  //if the msg isnt a request send error 
  if(READ_TYPE != requestmsg.header && WRITE_TYPE != requestmsg.header){
    msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,"error on client request header: isn't read or write", buffer);
    EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));
    logError(ILLEGAL_OPERATION, "error on client request header: isn't read or write");
    exit(EXIT_FAILURE);
  }

  logs(NULL, hostName, hostIp, "UDP", port, 0,(requestmsg.header == READ_TYPE)? LOG_START_READ : LOG_START_WRITE);


  //check if file exists
  bool fileExists = (access( requestmsg.fileName, F_OK ) != -1) ? TRUE : FALSE;

  //obtain the request type and bifurcate 
  if(requestmsg.header == READ_TYPE){

      //send error if file not found
      if(!fileExists){
        sprintf(tag,"server couln't found %s" ,requestmsg.fileName);
        msgSize = fillBufferWithErrMsg(FILE_NOT_FOUND,tag, buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));
        logError(FILE_NOT_FOUND, tag);
        exit(EXIT_FAILURE);
      }

      //open the request file
      if(NULL == (f = fopen(requestmsg.fileName,"rb"))){
        //if error send error msg 
        sprintf(tag,"server couln't found %s" ,requestmsg.fileName);
        msgSize = fillBufferWithErrMsg(UNKNOWN, tag, buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));
        logError(UNKNOWN, tag);
        exit(EXIT_FAILURE);
      }

      while(!endSesion){
        //send block 
          memset(dataBuffer,0,MSG_DATA_SIZE);
          readSize = fread(dataBuffer, 1, MSG_DATA_SIZE, f);
          if(-1 == readSize){
            sprintf(tag,"error reading the file %s" ,requestmsg.fileName);
            msgSize = fillBufferWithErrMsg(UNKNOWN,tag, buffer);
            EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));
            logError(UNKNOWN, tag);
            exit(EXIT_FAILURE);
          }

          msgSize = fillBufferWithDataMsg(blockNumber,dataBuffer,readSize,buffer);
          EXIT_ON_WRONG_VALUE(TRUE,"Error on sending data block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));

          logs(requestmsg.fileName,hostName, hostIp, "UDP", port, blockNumber,LOG_READ);

        //wait for ack 
          msgSize = reciveUdpMsg(s,buffer,&clientaddr_in);
          if(msgSize < 0){
            msgSize = fillBufferWithErrMsg(UNKNOWN,"Error on receiving ack", buffer);
            EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));
            logError(UNKNOWN, "Error on receiving ack");
            exit(EXIT_FAILURE);
          }

          switch(getMessageTypeWithBuffer(buffer)){
            case ACK_TYPE: 
              ackmsg = fillAckWithBuffer(buffer);
              //check if ack its correct; if not an error is send
              if( blockNumber != ackmsg.blockNumber ){
                msgSize = fillBufferWithErrMsg(UNKNOWN,"recived blockNumber doesn't match with current blockNumber", buffer);
                EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));
                logError(UNKNOWN, "recived blockNumber doesn't match with current blockNumber");
                exit(EXIT_FAILURE);
              }
              blockNumber += 1;
            break;
            case ERR_TYPE:
              errmsg = fillErrWithBuffer(buffer);
              sprintf(tag,"received error when waiting ack %d: %s",errmsg.errorCode, errmsg.errorMsg); 
              logError(errmsg.errorCode, tag);
              exit(EXIT_FAILURE);
            break;
            default:
              sprintf(tag,"unrecognized operation in current context %d" ,getMessageTypeWithBuffer(buffer));
              msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,tag, buffer);
              EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));
              logError(ILLEGAL_OPERATION, tag);
              exit(EXIT_FAILURE);
            break; 
          }

        //check if the file has ended
        if(feof(f))
          endSesion = TRUE;
      }

  }else{
      //send error if file found
    /* if(fileExists){
        sprintf(tag,"the requested file alerady exists: %s" ,requestmsg.fileName);
        msgSize = fillBufferWithErrMsg(FILE_ALREADY_EXISTS,tag, buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error becuase file exists",(sendto(s, buffer, msgSize, 0) != msgSize));
        logError(FILE_ALREADY_EXISTS, tag);
        exit(EXIT_FAILURE);
      }*/

      //open the request file
      char destionationFile[200];
      sprintf(destionationFile,"log/%s",requestmsg.fileName);
      if(NULL == (f = fopen(destionationFile,"wb+"))){
        //if error send error msg
        sprintf(tag,"error opening the file %s" ,requestmsg.fileName);
        msgSize = fillBufferWithErrMsg(UNKNOWN,tag, buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));
        logError(UNKNOWN, tag);
        exit(EXIT_FAILURE);
      }

      //send ack and increment block
      msgSize = fillBufferWithAckMsg(datamsg.blockNumber, buffer);
      EXIT_ON_WRONG_VALUE(TRUE,"Error on sending ack for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));
      blockNumber += 1;
      
      while(!endSesion){
      //recive block
      msgSize = reciveUdpMsg(s,buffer,&clientaddr_in);
      if(msgSize < 0){
        msgSize = fillBufferWithErrMsg(UNKNOWN,"Error on reciving block", buffer);
        EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error msg",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));
        logError(UNKNOWN, "Error on reciving block");
        exit(EXIT_FAILURE);
      }

      //act according to type
      switch(getMessageTypeWithBuffer(buffer)){
        case DATA_TYPE:

          datamsg = fillDataWithBuffer(msgSize,buffer);

          //check if block number is the correct one
          if(datamsg.blockNumber != blockNumber ){
            msgSize = fillBufferWithErrMsg(UNKNOWN,"UNKNOWN", buffer);
              EXIT_ON_WRONG_VALUE(TRUE,"recived blockNumber doesn't match with current blockNumber",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));
              logError(UNKNOWN, "recived blockNumber doesn't match with current blockNumber");
              exit(EXIT_FAILURE);
          }
          //write the send data
          writeResult = fwrite(datamsg.data,1,DATA_SIZE(msgSize),f);
          if(-1 == writeResult){
            sprintf(tag,"error writing the file %s" ,requestmsg.fileName);
            msgSize = fillBufferWithErrMsg(DISK_FULL,tag, buffer);
              EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));
              logError(DISK_FULL, tag);
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
            logError(errmsg.errorCode, tag);
            exit(EXIT_FAILURE);
        break;
        default:
              sprintf(tag,"unrecognized operation in current context %d" ,getMessageTypeWithBuffer(buffer));
              msgSize = fillBufferWithErrMsg(ILLEGAL_OPERATION,tag, buffer);
              EXIT_ON_WRONG_VALUE(TRUE,"Error on sending error for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));
              logError(ILLEGAL_OPERATION, tag);
              exit(EXIT_FAILURE);
        break; 
      }

      //send ack
      msgSize = fillBufferWithAckMsg(datamsg.blockNumber, buffer);
      EXIT_ON_WRONG_VALUE(TRUE,"Error on sending ack for block",(sendto(s, buffer, msgSize, 0,(struct sockaddr *)&clientaddr_in,sizeof(struct sockaddr_in)) != msgSize));

      //log received data 
      logs(requestmsg.fileName,hostName, hostIp, "UDP", port, blockNumber, LOG_WRITE);
    }
  }

  fclose(f);
  close(s);
  logs(requestmsg.fileName,hostName, hostIp, "UDP", port, 0, LOG_END);
}

void SIGTERMHandler(int ss){
    end = TRUE;
}

void SIGALRMHandler(int ss){
	logError(0, "the timeout passed");
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

//=================================================================================================

const char * getDateAndTime(){
	static char timeString[TIME_STRING_SIZE];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(timeString,"%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	return timeString;
}

void logIssue(char * issue){
  char toLog[LOG_MESSAGE_SIZE];
  FILE*logFile = fopen(SERVER_LOG,"a+");
  sprintf(toLog,"\n[%s][ISSUE][%s]",getDateAndTime(),issue);
  fprintf(logFile, "%s",toLog);
  fclose(logFile);
}

void logError(int errorcode, char * errorMsg){
	char toLog[LOG_MESSAGE_SIZE];
    char error[30];
	FILE*logFile = fopen(SERVER_LOG,"a+");
    
    switch (errorcode) {
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

	sprintf(toLog,"\n[%s][ERROR: %s %s]",getDateAndTime(),error,errorMsg);
	fprintf(stderr,"%s",toLog);
	fprintf(logFile, "%s",toLog);

	fclose(logFile);
}

void logs(char * file, char * host, char * ip, char * protocol, int clientPort, int blockNumber, int mode){
	char toLog[LOG_MESSAGE_SIZE];
	FILE*logFile = fopen(SERVER_LOG,"a+");

  switch(mode){
    case LOG_START_READ: sprintf(toLog,"\n[%s][Host: %s][IP:%s][Protocol:%s][Port:%d][CONNECTION STARTED][READ MODE]",getDateAndTime(),host, ip, protocol, clientPort); break;
    case LOG_START_WRITE: sprintf(toLog,"\n[%s][Host: %s][IP:%s][Protocol:%s][Port:%d][CONNECTION STARTED][WRITE MODE]",getDateAndTime(),host, ip, protocol, clientPort); break;
    case LOG_READ: sprintf(toLog,"\n[%s][Host: %s][IP:%s][Protocol:%s][Port:%d][File %s][Send block:%d]",getDateAndTime(),host, ip, protocol, clientPort,file,blockNumber); break;
    case LOG_WRITE: sprintf(toLog,"\n[%s][Host: %s][IP:%s][Protocol:%s][Port:%d][File %s][Recived block:%d]",getDateAndTime(),host, ip, protocol, clientPort,file,blockNumber); break;
    case LOG_END: sprintf(toLog,"\n[%s][Host: %s][IP:%s][Protocol:%s][Port:%d][File %s][SUCCED]",getDateAndTime(),host, ip, protocol, clientPort,file); break;
  }

	fprintf(stderr,"%s",toLog);
	fprintf(logFile, "%s",toLog);

  fclose(logFile);
}
