/*
 ** Fichero: cliente.c
 ** Autores:
 ** Julio García Valdunciel DNI 70892288D
 ** David Flores Barbero DNI 70907575R
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
#include <signal.h>
#include <sys/errno.h>


#define PUERTO 7575
#define TAM_BUFFER 1024
#define ADDRNOTFOUND 0xffffffff	/* value returned for unknown host */
#define RETRIES	5		/* number of times to retry before givin up */
#define TIMEOUT 6
#define MAXHOST 512


typedef struct tipoMensaje{
	char orden[8];
	char pagina[50];
	char host[50];
	char keepAlive[16];
} Mensaje;

int errno;

/*----------------------------------------*/
handler(){
/*----------------------------------------*/

 printf("Alarma recibida \n");
}

char *construirMensaje(char *lineaOrden, Mensaje *msg);

/*----------------------------------------*/
int main(int argc, char *argv[]){
/*----------------------------------------*/

char protocolo[5];
char archivoOrdenes[20];

	int s;							/* socket descriptor */
	struct sockaddr_in myaddr_in;	/* local socket address */
    struct sockaddr_in servaddr_in;	/* server socket address */
	struct addrinfo hints, *res;
	struct in_addr reqaddr;			/* for returned internet address */
	socklen_t addrlen;
	int tamMsg;
	time_t timevar;
	int errcode;
	int n_retry = RETRIES;//numero de intentos udp

	char buf[TAM_BUFFER];			/* Buffer */
	char hostname[MAXHOST];    		/*nombre del host*/
	struct sigaction vec;

	//Tratamiento del fichero
	FILE * fileOrdenes;
	char lineaOrden[30];

	Mensaje msg;

	FILE *fileLogCliente;
	char puertoEfimero[10];


	//mensaje enviado al servidor
	char *mensajePeticion;


	//Recibir mensaje
	char tok[50];
	char html[200];

	if(argc !=4){
		printf("Uso");
		exit(1);
	}



	sprintf(msg.host, "%s", argv[1]);
	strcpy(protocolo, argv[2]);
	strcpy(archivoOrdenes, argv[3]);


	if(strcmp(protocolo,"TCP") == 0 ){

	/*-------------------------------------------------*/
	/*			C L I E N T  T C P					   */
	/*												   */
	/*-------------------------------------------------*/

		


		/* Create the socket. */
		s = socket (AF_INET, SOCK_STREAM, 0);
		if (s == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to create socket\n", argv[0]);
			exit(1);
		}

			/* clear out address structures */
		memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
		memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

		/* Set up the peer address to which we will connect. */
		servaddr_in.sin_family = AF_INET;

		/* Get the host information for the hostname that the
		 * user passed in. */
		memset (&hints, 0, sizeof (hints));
		hints.ai_family = AF_INET;

	 	 /* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
		//obtiene la direccion asociada al nombre
		errcode = getaddrinfo (argv[1], NULL, &hints, &res);
		if (errcode != 0){
				/* Name was not found.  Return a
				 * special value signifying the error. */
			fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
					argv[0], argv[1]);
			exit(1);
		}
		else {
			/* Copy address of host */
			servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
		}
		freeaddrinfo(res);

		/* puerto del servidor en orden de red*/
		servaddr_in.sin_port = htons(PUERTO);

		//conectar con la direccion del socket remoto
		if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
			exit(1);
		}

		/* Since the connect call assigns a free address
			 * to the local end of this connection, let's use
			 * getsockname to see what it assigned.  Note that
			 * addrlen needs to be passed in as a pointer,
			 * because getsockname returns the actual length
			 * of the address.
			 */
		addrlen = sizeof(struct sockaddr_in);

		//getsockename devuelve la informacion del sock creado localmente en la maquina del cliente
		if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
			exit(1);
		}


		/* Print out a startup message for the user. */
		time(&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
		printf("Conectado a %s en el puerto %u hora %s", argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

		/* TODO	LEER ORDENES DE FICHERO */
		//TEST ENVIO

		sprintf(puertoEfimero,"%d",ntohs(myaddr_in.sin_port));
		strcat(puertoEfimero,".txt");
		if(NULL == (fileLogCliente = fopen(puertoEfimero,"a+"))){
			fprintf(stderr,"No se pudo crear el fichero de log\n");
			exit(1);
		}


		if(NULL == (fileOrdenes = fopen(archivoOrdenes,"r"))){
			fprintf(fileLogCliente,"No se pudo abrir archivo de ordenes\n");
			exit(1);
		}

		fgets(lineaOrden, sizeof(lineaOrden), fileOrdenes);
		while(!feof(fileOrdenes)){

			mensajePeticion = construirMensaje(lineaOrden, &msg);

fprintf(stderr,"Peticion n:%lu:\n%s",strlen(mensajePeticion),mensajePeticion);
printf("Peticion: %s %s\n",msg.orden, msg.pagina);
			if (send(s, mensajePeticion, strlen(mensajePeticion), 0) == -1) {
					fprintf(fileLogCliente, "%s: Connection aborted on error\n",	argv[0]);
					exit(1);
			}

			free(mensajePeticion);

			if(msg.keepAlive[0] != 'k'){

				if (shutdown(s, 1) == -1) {
					perror(argv[0]);
					fprintf(fileLogCliente, "%s: unable to shutdown socket\n", argv[0]);
					exit(1);
				}
				
			}

			if ((tamMsg = recv(s, buf, TAM_BUFFER, 0)) == -1) {
			    perror(argv[0]);
				fprintf(fileLogCliente, "%s: error reading result\n", argv[0]);
				exit(1);
			}
			buf[tamMsg]='\0';
//fprintf(stderr,"Cliente recibe n:%lu:\n%s\n",strlen(buf),buf);
			
			fprintf(fileLogCliente,"Respuesta servidor:\n%s\n\n",buf);



			sscanf(buf,"%[^'\r']\r\n%[^'\r']\r\n%[^'\r']\r\n%[^'º']",tok,tok,tok,html);
			printf("%s\n\n",html);			

			fgets(lineaOrden, sizeof(lineaOrden), fileOrdenes);		
		}
		fclose(fileOrdenes);
		time(&timevar);
		printf("All done at %s", (char *)ctime(&timevar));

	}else if(strcmp(protocolo,"UDP") == 0){

	/*-------------------------------------------------*/
	/*			C L I E N T  U D P					   */
	/*												   */
	/*-------------------------------------------------*/


		/* Create the socket. */
		s = socket (AF_INET, SOCK_DGRAM, 0);
		if (s == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to create socket\n", argv[0]);
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

		//bind para asociar la info del socket local con la de la estructra myaddr_in
		if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to bind socket\n", argv[0]);
			exit(1);
		}

		//tamaño de la direccion
		addrlen = sizeof(struct sockaddr_in);

		//dsvuelve la direccion actual del socket s en myaddr_in
		if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
			    perror(argv[0]);
			    fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
			    exit(1);
		}


         /* Mensaje incial para el usuario */
		time(&timevar);
            /* el puerto debe ser convertido antes(ntohs)
             */
		printf("Connected to %s on port %u at %s", argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));


		/* prepara la direccion del servidor */
		servaddr_in.sin_family = AF_INET;

		/* Get the host information for the server's hostname that the
		* user passed in.
		*/
		memset (&hints, 0, sizeof (hints));
		hints.ai_family = AF_INET;

		/* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/

		//coloca en la estructura res la informacion de la direccion usada posteriormente
		errcode = getaddrinfo (argv[1], NULL, &hints, &res);
		if (errcode != 0){
			/* Name was not found.  Return a
			 * special value signifying the error. */
			fprintf(stderr, "%s: No es posible resolver la IP de %s\n",	argv[0], argv[1]);
			exit(1);
		}else {

			/* Obtiene la direccion del host */
			servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
		}
		freeaddrinfo(res);
		/* puerto del servidor en orden de red*/
		servaddr_in.sin_port = htons(PUERTO);

	   /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
	  	vec.sa_handler = (void *) handler;
		vec.sa_flags = 0;
		if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
		        perror(" sigaction(SIGALRM)");
		        fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
		        exit(1);
		}

		sprintf(puertoEfimero,"%d",ntohs(myaddr_in.sin_port));
		strcat(puertoEfimero,".txt");

		if(NULL == (fileOrdenes = fopen(archivoOrdenes,"r"))){
			fprintf(fileLogCliente,"No se pudo abrir archivo de ordenes\n");
			exit(1);
		}
		fgets(lineaOrden, sizeof(lineaOrden), fileOrdenes);
		while(!feof(fileOrdenes)){
		n_retry=RETRIES;
			
			while (n_retry > 0) {
				/* Send the request to the nameserver. */
				//segundo argumento son los datos a enviar
				//el 5 argumento contiene los datos del socket remoto
				mensajePeticion = construirMensaje(lineaOrden, &msg);

				if(NULL == (fileLogCliente = fopen(puertoEfimero,"a+"))){
					fprintf(stderr,"No se pudo crear el fichero de log\n");
					exit(1);
				}

				fprintf(fileLogCliente,"Cliente envia:\n%s\n",mensajePeticion);
				fprintf(stderr,"Cliente envia:\n%s\n",mensajePeticion);
				fclose(fileLogCliente);

				if (sendto(s, mensajePeticion ,strlen(mensajePeticion) ,0 ,(struct sockaddr *)&servaddr_in ,sizeof(struct sockaddr_in)) == -1) {

						perror(argv[0]);
						fprintf(stderr, "%s: unable to send request\n", argv[0]);
						exit(1);
				}
				/* establece un timeout y salta una alarma cada vez que se alcanza
				 */
				alarm(TIMEOUT);

				//recibe los datos recogiendo la direccion del socket remoto
				//es bloqueante mirar timeouts
				if ((tamMsg = recvfrom (s, buf, sizeof(buf), 0, (struct sockaddr *)&servaddr_in, &addrlen)) == -1) {
					if (errno == EINTR) {
							/* Alarm went off and aborted the receive.
							 * Need to retry the request if we have
							 * not already exceeded the retry limit.
							 */
						printf("attempt %d (retries %d).\n", n_retry, RETRIES);
		  	 			n_retry--;
					}else{
						printf("Unable to get response from");
						exit(1);
					}

				}else {
				    alarm(0);
				  
					buf[tamMsg-1]='\0';

					if(NULL == (fileLogCliente = fopen(puertoEfimero,"a+"))){
						fprintf(stderr,"No se pudo crear el fichero de log\n");
						exit(1);
					}
					fprintf(fileLogCliente,"Cliente recibe:\n%s\n\n",buf);
					fprintf(stderr,"Cliente recibe:\n%s\n\n",buf);

					fclose(fileLogCliente);
					memset(buf,0,sizeof(buf));
					break;
				}

			}//fin del while(retries)
			if (n_retry == 0) {
				printf("Unable to get response from %s after %d attempts.\n", argv[1], RETRIES);
				printf("La comunicacion se cerrara");
				break;
			}

			fgets(lineaOrden, sizeof(lineaOrden), fileOrdenes);
		}

	//se llega al maximo de intentos
		

	}else{
fprintf(stderr,"error protocolo");
		perror("ERROR protocolo");
		exit(1);
	}

	return 0;
}

char * construirMensaje(char *lineaOrden, Mensaje *msg){

	char c;
	int indLinea = 0, indCampo = 0;
	char *mensajePeticion;

	mensajePeticion = (char *)malloc(100*sizeof(char));
	
	memset(msg->orden, 0, sizeof(msg->orden));
	memset(msg->pagina, 0, sizeof(msg->pagina));
	memset(msg->keepAlive, 0, sizeof(msg->keepAlive));

	sscanf(lineaOrden,"%[^' '] %[^' '] %[^'\n']\n",msg->orden, msg->pagina, msg->keepAlive);

	//Construccion del mensaje que se le mandara al servidor
	strcpy(mensajePeticion, msg->orden);
	strcat(mensajePeticion, " ");
	strcat(mensajePeticion, msg->pagina);
	strcat(mensajePeticion, " ");
	strcat(mensajePeticion, "HTTP/1.1\r\n");
	strcat(mensajePeticion, "Host: ");
	strcat(mensajePeticion, msg->host);
	strcat(mensajePeticion, "\r\n");

	if(msg->keepAlive[0] == 'k'){
		strcat(mensajePeticion, "Connection: keep-alive\r\n");
	}else {
		strcat(mensajePeticion, "Connection: close\r\n");
	}
	strcat(mensajePeticion, "\r\n");

	return mensajePeticion;
}



