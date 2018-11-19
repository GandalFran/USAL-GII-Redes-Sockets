/*

** Fichero: logUtils.c
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/

#include <sys/types.h>
#include <unistd.h>

#define SERVER_HOST_NAME "olivo.fis.usal.es"
#define SERVER_IP

#define SERVER_LOG "peticiones.log"

#define BUFF 1000
#define DATE_AND_TIME_TAM 20
#define CLIENT_FILE_PATH_SIZE 100


FILE * serverLog = openFile(SERVER_LOG);

//NOTA: host es el nombre del cliente
void logServer(char*ip, char * protocol, char * clientPort){
	char toLog[BUFF];
	char dateAndTime[DATE_AND_TIME_TAM];
	
	getDateAndTime(dateAndTime);
	sprintf(toLog,"\n[%s][Connection] Host:%20s IP:%20s Protocol:%20s ClientPort:%20s",dateAndTime,SERVER_HOST_NAME,ip,protocol,clientPort);	

	fprintf(stderr,toLog);
	fwrite(toLog, sizeof(char), BUFF, serverLog);
}

void logServerEnded(char * ip, char * protocol, char * clientPort,int error){
	char toLog[BUFF];
	char dateAndTime[DATE_AND_TIME_TAM];
	
	getDateAndTime(dateAndTime);	
	if(error){
		sprintf(toLog,"\n[%s][File][ERROR: %s][Host:%20s IP:%20s Protocol:%20s ClientPort:%20s]",,error,SERVER_HOST_NAME,ip,protocol,clientPort);	
	}else{
		sprintf(toLog,"\n[%s][File][SUCCED][Host:%20s IP:%20s Protocol:%20s ClientPort:%20s]",,SERVER_HOST_NAME,ip,protocol,clientPort);	
	}

	fprintf(stderr,toLog);
	fwrite(toLog, sizeof(char), BUFF, serverLog);
}


void logClient(char * fileName,int block){
	char toLog[BUFF];
	char dateAndTime[DATE_AND_TIME_TAM];
	char filePath[CLIENT_FILE_PATH_SIZE];

	sprintf(filePath,"[]");

	getDateAndTime(dateAndTime);
	sprintf(toLog,"\n[%s][File %15s] Block:%d",dateAndTime,fileName,block);	

	fprintf(stderr,toLog);
	fwrite(toLog, sizeof(char), BUFF, serverLog);

Enviando el fichero nombre-fichero, enviando bloque 1, enviando bloque 2, enviando bloque 3 y último,
finalizado correctamente

Escribirá
los mensajes de progreso y los mensajes de
error y/o depuración en un fichero con nombre el
número de puerto efímero del cliente y extensión .txt
}

void openFile(){

}

void getDateAndTime(char * toPrint){
	struct tm * tm;
	tm = (&time(NULL));
	sprintf(toPrint,"%s%d"...);
}
