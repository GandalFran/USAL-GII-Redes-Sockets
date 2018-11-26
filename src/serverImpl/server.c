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
#include "../utils/logUtils.h"
#include "../utils/msgUtils.h"
#include "../utils/concurrencyUtils.h"

#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define MAXHOST 128

//socket descriptors and bool to check which one should be close in SIGTERMHandler
int udpDescriptor, tcpDescriptor, listenTcpDescriptor;

//signal handlers
void SIGTERMHandler(int);
void SIGALRMHandler(int);
//servers impl
void tcpServer(int s, struct sockaddr_in clientaddr_in);
void udpServer(int s, char * buffer, struct sockaddr_in clientaddr_in);

void initializeServers();

int main(int argc, char * argv[]){
  //Block all signals and redefine sigterm
    sigset_t signalSet;
    EXIT_ON_FAILURE(-1,sigfillset(&signalSet));
    EXIT_ON_FAILURE(-1,sigprocmask(SIG_SETMASK, &signalSet, NULL));
    redefineSignal(SIGTERM,SIGTERMHandler);
  //Server initialization -- in a function because is taken from Nines template
    initializeServers();
}

void initializeServers(){

      int tcpDescriptor, udpDescriptor;		/* connected socket descriptor */
      int listenTcpDescriptor;				/* listen socket descriptor */

      int cc;				    /* contains the number of bytes read */

      struct sockaddr_in myaddr_in;	/* for local socket address */
      struct sockaddr_in clientaddr_in;	/* for peer socket address */
  	int addrlen;

      fd_set readmask;
      int numfds,s_mayor;

      char buffer[BUFFERSIZE];	/* buffer for packets to be read into */

      struct sigaction vec;

  	/* Create the listen socket. */
  	EXIT_ON_FAILURE(-1,listenTcpDescriptor = socket (AF_INET, SOCK_STREAM, 0));
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
  	EXIT_ON_FAILURE(-1, bind(listenTcpDescriptor, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) );
  		/* Initiate the listen on the socket so remote users
  		 * can connect.  The listen backlog is set to 5, which
  		 * is the largest currently supported.
  		 */
  	EXIT_ON_FAILURE(-1,listen(listenTcpDescriptor, 5));

  	/* Create the socket UDP. */
  	EXIT_ON_FAILURE(-1,udpDescriptor = socket (AF_INET, SOCK_DGRAM, 0));

  	/* Bind the server's address to the socket. */
  	EXIT_ON_FAILURE(-1,bind(udpDescriptor, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)));

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

    pid_t daemonPid = createProcess();
    if(!isChild(daemonPid)){
      /*If is father it exits*/
      exit(EXIT_SUCCESS);
    }else{
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


	    while (TRUE) {
	      /* Meter en el conjunto de sockets los sockets UDP y TCP */
	      FD_ZERO(&readmask);
	      FD_SET(listenTcpDescriptor, &readmask);
	      FD_SET(udpDescriptor, &readmask);
	      /*Seleccionar el descriptor del socket que ha cambiado. Deja una marca en
	      el conjunto de sockets (readmask)
	      */

	      s_mayor = (listenTcpDescriptor > udpDescriptor) ? listenTcpDescriptor : udpDescriptor;

	      EXIT_ON_FAILURE(-1,numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL));

	      /* Comprobamos si el socket seleccionado es el socket TCP */
	      if (FD_ISSET(listenTcpDescriptor, &readmask)) {
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
		  EXIT_ON_FAILURE(-1,tcpDescriptor = accept(listenTcpDescriptor, (struct sockaddr *) &clientaddr_in, &addrlen));

		  pid_t serverInstancePid = createProcess();
		  if(isChild(serverInstancePid)){

		    close(listenTcpDescriptor); /* Close the listen socket inherited from the daemon. */
		    tcpServer(tcpDescriptor, clientaddr_in);
		    exit(EXIT_SUCCESS);
		  }else{
		    close(tcpDescriptor);
		  }
	      }

	      /* Comprobamos si el socket seleccionado es el socket UDP */
	      if (FD_ISSET(udpDescriptor, &readmask)) {
	//FALTA POR HACER UDP
		    /* This call will block until a new
		        * request arrives.  Then, it will
		        * return the address of the client,
		        * and a buffer containing its request.
		        * BUFFERSIZE - 1 bytes are read so that
		        * room is left at the end of the buffer
		        * for a null character.
		        */
		    EXIT_ON_FAILURE(-1,cc = recvfrom(udpDescriptor, buffer, BUFFERSIZE - 1, 0, (struct sockaddr *)&clientaddr_in, &addrlen));
		        /* Make sure the message received is
		        * null terminated.
		        */
		   buffer[cc]='\0';
		   udpServer (udpDescriptor, buffer, clientaddr_in);
	     }
	    
	   }/* Fin del bucle infinito de atenci�n a clientes */
  }
}

void SIGTERMHandler(int ss){
  //close handlers
  close(tcpDescriptor);
  close(udpDescriptor);
  close(listenTcpDescriptor);
  //close log
  closeServerLog();
  //exit
  exit(EXIT_SUCCESS);
}

void SIGALRMHandler(int ss){

}

void tcpServer(int s, struct sockaddr_in clientaddr_in){
  logServer(inet_ntoa(clientaddr_in.sin_addr), "TCP", clientaddr_in.sin_port, FALSE, FALSE, NULL);
  logServer(inet_ntoa(clientaddr_in.sin_addr), "TCP", clientaddr_in.sin_port, TRUE, FALSE, NULL);
}

void udpServer(int s, char * buffer, struct sockaddr_in clientaddr_in){
 static dataMsg * ultimasPeticiones[BUFFER_SIZE];
  fprintf(stderr, "%s\n","UDP SERVER");
}
