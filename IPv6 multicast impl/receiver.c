/*
** Fichero: receiver.c
** Autores:
** Francisco Pinto Santos DNI 70918455W
** Usuario: i0918455
** Hector Sanchez San Blas DNI 70901148Z
** Usuario: i0901148
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <net/if.h>

#define TRUE 1

#define DEFAULT_PORT 8455
#define DEFAULT_MULTICAST_GROUP "ff15::33"
#define DEFAULT_IFACE "lo"

#define DEFAULT_SIZE 80
#define MAX_MESSAGE_SIZE 4096

#define EXIT_ON_FAILURE(returnValue)            									\
    do{                                         									\
        if((returnValue) == -1){                									\
        	char errorTag[80];                                                      \
            sprintf(errorTag, "\n[%s:%d:%s] ", __FILE__, __LINE__, __FUNCTION__);   \
            perror(errorTag); 														\
            exit(EXIT_FAILURE);                 									\
        }                                       									\
    }while(0)


void checkargs(int argc, char *argv[], char * group, char * iface, int * port);
void registersignals();
void protocolImpl(char * group, char * iface, int port);
void recivemessage(int s);

void SIGINThandler(int ss);

//socket
int	s;

int main(int argc, char *argv[]){
	int port;
	char group[DEFAULT_SIZE], iface[DEFAULT_SIZE];

	checkargs(argc,argv,group,iface,&port);

	registersignals();

	fprintf(stderr,"==============================\n\tgrupo:\t\t%s\n\tinterfaz:\t%s\n\tUDP puerto:\t%d\n==============================\n",group,iface,port);

	protocolImpl(group,iface,port);
}


void checkargs(int argc, char *argv[], char * group, char * iface, int * port){
	//check args
	if(argc != 4 && argc != 1){
		fprintf (stderr,"Uso: %s <IPv6multicast> <interfaz> <puerto>\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	//fill internal vars
	if(argc == 4){
		strcpy(group,argv[1]);
		strcpy(iface,argv[2]);
		*port = atoi(argv[3]);
	}else{	
		strcpy(group,DEFAULT_MULTICAST_GROUP);
		strcpy(iface,DEFAULT_IFACE);
		*port = DEFAULT_PORT;
	}
}

void registersignals(){
	sigset_t sigset;
	struct sigaction ss;

	//fill masks
	EXIT_ON_FAILURE(sigfillset(&sigset));
	EXIT_ON_FAILURE(sigdelset(&sigset, SIGINT));

	//redefine sigint
    ss.sa_flags = 0;
    ss.sa_handler = SIGINThandler;
	EXIT_ON_FAILURE(sigfillset(&ss.sa_mask));
    EXIT_ON_FAILURE(sigaction(SIGINT,&ss,NULL));
}

void protocolImpl(char * group, char * iface, int port){
	//socket data
	struct ipv6_mreq ipv6mreq;
	struct sockaddr_in6 suscriptorData;

	//reset structures
	memset(&ipv6mreq, 0, sizeof(ipv6mreq));
	memset(&suscriptorData, 0, sizeof(suscriptorData));

	//fill structures
		//fill suscriptorData structure: to bind the socket to suscriptor address
		suscriptorData.sin6_family = AF_INET6;
		suscriptorData.sin6_port = htons(port);
		suscriptorData.sin6_addr = in6addr_any;
		//fill ipv6mreq structure: to join the group
		ipv6mreq.ipv6mr_interface = if_nametoindex(iface);
		EXIT_ON_FAILURE( inet_pton(AF_INET6, group, &ipv6mreq.ipv6mr_multiaddr) );

	//get socket
	EXIT_ON_FAILURE( s = socket(AF_INET6, SOCK_DGRAM, 0) );

	//set SO_REUSERADDR to enable multiple suscriptors in one host
	int enable = 1;
	EXIT_ON_FAILURE( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)));

	//bind socket to our address
	EXIT_ON_FAILURE( bind(s, (const struct sockaddr *) &suscriptorData, sizeof(suscriptorData)) );

	//join group
	EXIT_ON_FAILURE( setsockopt(s, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &ipv6mreq, sizeof(ipv6mreq)));

	//recive data until CTRL+C
	while(TRUE){
		recivemessage(s);
	}
}

void recivemessage(int s){
	static int messageid = 0;

	int size, addrlen = sizeof(struct sockaddr_in6);
	struct sockaddr_in6 senderData;
	char message[MAX_MESSAGE_SIZE], senderipv6[INET6_ADDRSTRLEN];

	//reset message and buffer
	memset(message, 0 , MAX_MESSAGE_SIZE *sizeof(char));
	memset(senderipv6, 0 ,INET6_ADDRSTRLEN*sizeof(char));
	memset(&senderData, 0 ,sizeof(senderData));

	//recive message in buffer
	size = recvfrom(s, message, MAX_MESSAGE_SIZE, 0,(struct sockaddr *)&senderData, &addrlen);

	if(size > 0){
		inet_ntop(AF_INET6, &(senderData.sin6_addr), senderipv6, INET6_ADDRSTRLEN*sizeof(char));
		fprintf(stderr,"Nuevo mensaje [%d] \"%s\" de %s\n",messageid++,message,senderipv6);
	}
}

void SIGINThandler(int ss){
	close(s);
	fprintf(stderr, "\nLa aplicacion termino por CTRL+C\n");
	exit(EXIT_SUCCESS);
}


