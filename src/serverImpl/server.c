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
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../utils/utils.h"
#include "../utils/msgUtils.h"

#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define MAXHOST 128

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
void logError(char * errorMsg);
void logs(char * ip, char * protocol, int clientPort, bool end);

#define EXIT_ON_WRONG_VALUE(wrongValue, errorMsg, returnValue)                          \
do{                                              		    	                              \
    if((returnValue) == (wrongValue)){		    	                                        \
        char errorTag[200];                                                             \
        sprintf(errorTag, "\n[%s:%d:%s]%s", __FILE__, __LINE__, __FUNCTION__,errorMsg); \
        logError(errorTag);		    	                                                    \
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

      char buffer[BUFFERSIZE];	/* buffer for packets to be read into */

      struct sigaction vec;

  	/* Create the listen socket. */
  	EXIT_ON_WRONG_VALUE(-1,"Unable to create socket listen TCP",ls_TCP = socket (AF_INET, SOCK_STREAM, 0));
  	/* clear out address structures */
  	memset (&myaddr_in, 0, sizeof(struct sockaddr_in));
    memset (&clientaddr_in, 0, sizeof(struct sockaddr_in));
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
  	EXIT_ON_WRONG_VALUE(-1,"unable to bind address TCP",bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) );
  		/* Initiate the listen on the socket so remote users
  		 * can connect.  The listen backlog is set to 5, which
  		 * is the largest currently supported.
  		 */
  	EXIT_ON_WRONG_VALUE(-1,"unable to listen on socket",listen(ls_TCP, 5));

  	/* Create the socket UDP. */
  	EXIT_ON_WRONG_VALUE(-1,"unable to create socket UDP",s_UDP = socket (AF_INET, SOCK_DGRAM, 0));

  	/* Bind the server's address to the socket. */
  	EXIT_ON_WRONG_VALUE(-1,"unable to bind address UDP",bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)));

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
               * BUFFERSIZE - 1 bytes are read so that
               * room is left at the end of the buffer
               * for a null character.
               */
              EXIT_ON_WRONG_VALUE(-1,"recvfrom error",cc = recvfrom(s_UDP, buffer, BUFFERSIZE - 1, 0, (struct sockaddr *)&clientaddr_in, &addrlen));
              /* Make sure the message received is
               * null terminated.
               */
              buffer[cc]='\0';
              udpServer (s_UDP, buffer, clientaddr_in);
          }
	    }

     //close handlers
     close(s_UDP);
     close(ls_TCP);
     exit(EXIT_SUCCESS);
}

//https://github.com/amit-mittal/TFTP/blob/master/tftp_s.c
void tcpServer(int s, struct sockaddr_in clientaddr_in){
  logs(inet_ntoa(clientaddr_in.sin_addr), "TCP", clientaddr_in.sin_port, FALSE);
  
  logs(inet_ntoa(clientaddr_in.sin_addr), "TCP", clientaddr_in.sin_port, TRUE);
}

void udpServer(int s, char * buffer, struct sockaddr_in clientaddr_in){
  static dataMsg * ultimasPeticiones[BUFFERSIZE];
  fprintf(stderr, "%s\n","UDP SERVER");
}

void SIGTERMHandler(int ss){
    end = TRUE;
}

void SIGALRMHandler(int ss){

}

void redefineSignal(int signal, void(*function)(int)){
  struct sigaction ss;
  memset(&ss,0,sizeof(ss));

  ss.sa_handler=function;
  ss.sa_flags=0;

  EXIT_ON_WRONG_VALUE(-1,"signal error",sigfillset(&ss.sa_mask));
  EXIT_ON_WRONG_VALUE(-1,"signal error",sigaction(signal,&ss,NULL));
}

const char * getDateAndTime(){
	static char timeString[TIME_STRING_SIZE];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(timeString,"%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	return timeString;
}

void logError(char * errorMsg){
	char toLog[LOG_MESSAGE_SIZE];
	FILE*logFile = fopen(SERVER_LOG_PATH,"a+");

	sprintf(toLog,"\n[%s][Connection][ERROR: %s]",getDateAndTime(),errorMsg);
	fprintf(stderr,"%s",toLog);
	fprintf(logFile, "%s",toLog);

	fclose(logFile);
}

void logs(char * ip, char * protocol, int clientPort, bool end){
	char toLog[LOG_MESSAGE_SIZE];
	FILE*logFile = fopen(SERVER_LOG_PATH,"a+");

	if(end){
			sprintf(toLog,"\n[%s][Connection][SUCCED][IP:%15s][Protocol:%3s][ClientPort:%5d]",getDateAndTime(),ip,protocol,clientPort);
	}else{
		  sprintf(toLog,"\n[%s][Connection][IP:%15s][Protocol:%3s][ClientPort:%5d]",getDateAndTime(),ip,protocol,clientPort);
	}
	fprintf(stderr,"%s",toLog);
	fprintf(logFile, "%s",toLog);

  fclose(logFile);
}
