/*
** Fichero: sender.c
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

#define DEFAULT_HOPS 16
#define DEFAULT_PORT 8455
#define DEFAULT_INTERVAL 1
#define DEFAULT_IFACE "lo"
#define DEFAULT_MESSAGE "Hola que tal"
#define DEFAULT_MULTICAST_GROUP "ff15::33"

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


#define DEBUG_LINE(msg)	fprintf(stderr,"\n[%s:%d:%s] %s", __FILE__, __LINE__, __FUNCTION__,msg);


void checkargs(int argc, char **argv, char * message, char * group, char * iface, int *port, int *hops, int *interval);
void registersignals();
void protocolImpl(char * message, char * group, char * iface, int port, int hops, int interval);
void sendmessage(int s, char * message, struct sockaddr_in6 groupData);

void SIGINThandler(int ss);

//socket
int s;

int main(int argc, char *argv[]){
	int port, hops, interval;
	char group[DEFAULT_SIZE], iface[DEFAULT_SIZE], message[DEFAULT_SIZE];

	checkargs(argc,argv,message,group,iface,&port,&hops,&interval);

	registersignals();

	fprintf(stderr,"==============================\n\tmensaje:\t%s\n\tintervalo:\t%d\n\tgrupo:\t\t%s\n\tinterfaz:\t%s\n\tUDP puerto:\t%d\n\tsaltos\t\t%d\n==============================\n",message,interval,group,iface,port,hops);

	protocolImpl(message, group, iface, port, hops, interval);
}


void checkargs(int argc, char *argv[], char * message, char * group, char * iface,int *port, int *hops, int *interval){
	
	//check args
	if(argc != 7 && argc != 1){
		fprintf (stderr, "Uso: %s <mensaje> <IPv6multicast> <interfaz> <puerto> <saltos> <intervalo>\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	//fill internal vars
	if(argc == 7){
		strcpy(message,argv[1]);
		strcpy(group,argv[2]);
		strcpy(iface,argv[3]);
		*port = atoi(argv[4]);
		*hops = atoi(argv[5]);
		*interval = atoi(argv[6]);
	}else{	
		strcpy(message,DEFAULT_MESSAGE);
		strcpy(group,DEFAULT_MULTICAST_GROUP);
		strcpy(iface,DEFAULT_IFACE);
		*port = DEFAULT_PORT;
		*hops = DEFAULT_HOPS;
		*interval = DEFAULT_INTERVAL;
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

void protocolImpl(char * message, char * group, char * iface, int port, int hops, int interval){
	//message data
	int n = 0;

	//socket data
	int	ifaceint;
	struct sockaddr_in6 senderData, groupData;

	//reset structures
	memset(&groupData, 0, sizeof(groupData));
	memset(&senderData, 0, sizeof(senderData));

	//fill structures
		//fill senderData structure: to bind the socket to suscriptor address
		senderData.sin6_family = AF_INET6;
		senderData.sin6_port = 0;	//puerto efimero
		senderData.sin6_addr = in6addr_any;
		//fill groupData structure: to send data to group
		groupData.sin6_family = AF_INET6;
		groupData.sin6_port = htons(port);
		EXIT_ON_FAILURE( inet_pton(AF_INET6, group, &groupData.sin6_addr) );

	//get socket
	EXIT_ON_FAILURE( s = socket(AF_INET6, SOCK_DGRAM, 0) );

	//bind socket to our address
	EXIT_ON_FAILURE( bind(s, (const struct sockaddr *) &senderData, sizeof(senderData)) );

	//set output interface
	ifaceint = if_nametoindex(iface);
	EXIT_ON_FAILURE(setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char *)&ifaceint, sizeof(ifaceint)));

	//set number of hops
	EXIT_ON_FAILURE(setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops)));

	//send data until CTRL+C
	while(TRUE){
		sendmessage(s,message,groupData);
		sleep(interval);
	}
}

void sendmessage(int s, char * message, struct sockaddr_in6 groupData){
	int messagesize = strlen(message);
	sendto(s,message,messagesize,0,(struct sockaddr *)&groupData,sizeof(groupData));
}

void SIGINThandler(int ss){
	close(s);
	fprintf(stderr, "\nLa aplicacion termino por CTRL+C\n");
	exit(EXIT_SUCCESS);
}


