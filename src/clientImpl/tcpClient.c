/*

** Fichero: tcpClient.c
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>

#include "tcpClient.h"
#include "clientLog.h"
#include "../utils/utils.h"
#include "../utils/msgUtils.h"

#define TAM_BUFFER 1024


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



void tcpReadMode(char * hostName, char * file){
    int s;				/* connected socket descriptor */
    struct addrinfo hints, *res;
    long timevar;			/* contains time returned by time() */
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in servaddr_in;	/* for server socket address */
	int addrlen, i, j, errcode;
    /* This example uses BUFFER_SIZE byte messages. */
	char buf[TAM_BUFFER];

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

	/* Print out a startup message for the user. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */

	printf("CLIENT  Connected to %s on port %u at %s", hostName, ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

//============================================ TFTP
send request
wait for response
try N times untill response or timeout


bool end = FALSE;
while(! end){
    recive
    send ack
    if error in some log and exit
}

log succed
//============================================END TFTP
//============================================ EXAMPLE
	for (i=1; i<=5; i++) {
		*buf = i;
		if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) {
			fprintf(stderr, "%s: Connection aborted on error ", "argv[0]");
			fprintf(stderr, "on send number %d\n", i);
			exit(1);
		}
	}


	/* Now, shutdown the connection for further sends.
	 * This will cause the server to receive an end-of-file
	 * condition after it has received all the requests that
	 * have just been sent, indicating that we will not be
	 * sending any further requests.
	 */
	EXIT_ON_WRONG_VALUE(-1,"unable to shutdown socket",shutdown(s, 1));

	/* Now, start receiving all of the replys from the server.
	 * This loop will terminate when the recv returns zero,
	 * which is an end-of-file condition.  This will happen
	 * after the server has sent all of its replies, and closed
	 * its end of the connection.
	 */
	while (i = recv(s, buf, TAM_BUFFER, 0)) {

		EXIT_ON_WRONG_VALUE(-1,"error reading result", i);
		/* The reason this while loop exists is that there
		 * is a remote possibility of the above recv returning
		 * less than BUFFER_SIZE bytes.  This is because a recv returns
		 * as soon as there is some data, and will not wait for
		 * all of the requested data to arrive.  Since BUFFER_SIZE bytes
		 * is relatively small compared to the allowed TCP
		 * packet sizes, a partial receive is unlikely.  If
		 * this example had used 2048 bytes requests instead,
		 * a partial receive would be far more likely.
		 * This loop will keep receiving until all BUFFER_SIZE bytes
		 * have been received, thus guaranteeing that the
		 * next recv at the top of the loop will start at
		 * the begining of the next reply.
		 */
		while (i < TAM_BUFFER) {
			j = recv(s, &buf[i], TAM_BUFFER-i, 0);
			EXIT_ON_WRONG_VALUE(-1,"error reading result", j);
			i += j;
		}
		/* Print out message indicating the identity of this reply. */
		printf("CLIENT Received result number %d\n", *buf);
	}

//============================================END EXAMPLE
	/* Print message indicating completion of task. */
	time(&timevar);
	printf("CLIENT  All done at %s", (char *)ctime(&timevar));
}

void tcpWriteMode(){

}
