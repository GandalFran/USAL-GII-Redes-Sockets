/*

** Fichero: logUtils.c
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "logUtils.h"

#define SERVER_LOG "log\\peticiones.log"
#define SERVER_HOST_NAME "olivo.fis.usal.es"

#define TIME_STRING_SIZE 100
#define LOG_MESSAGE_SIZE 1000
#define CLIENT_FILE_PATH_SIZE 20

FILE * serverLog = NULL, *clientLog = NULL;

const char * getDateAndTime(){
	static char timeString[TIME_STRING_SIZE];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(timeString,"%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	return timeString;
}

void openServerLog(){
	if(NULL != serverLog)
		fclose(serverLog);
	serverLog = fopen(SERVER_LOG,"a+");
}

void openClientLog(char * port){
	char path[CLIENT_FILE_PATH_SIZE];
	sprintf(path,"log\\%s.txt",port);

	if(NULL != clientLog)
		fclose(clientLog);
	clientLog = fopen(path,"w+");
}

void closeServerLog(){
	fclose(serverLog);
	serverLog = NULL;
}

void closeClientLog(){
	fclose(clientLog);
	clientLog = NULL;
}

void logServer(char * ip, char * protocol, char * clientPort, bool end, bool error, char * errorMsg){
	char toLog[LOG_MESSAGE_SIZE];

	if(NULL == serverLog)
		openServerLog();

	if(end){
		if(error)
			sprintf(toLog,"\n[%s][Connection][ERROR: %s][Host:%20s] [IP:%15s] [Protocol:%5s] [ClientPort:%5s]",getDateAndTime(),errorMsg,SERVER_HOST_NAME,ip,protocol,clientPort);
		else
			sprintf(toLog,"\n[%s][Connection][SUCCED][Host:%20s] [IP:%15s] [Protocol:%5s] [ClientPort:%5s]",getDateAndTime(),SERVER_HOST_NAME,ip,protocol,clientPort);
	}else{
		sprintf(toLog,"\n[%s][Connection] [Host:%20s] [IP:%15s] [Protocol:%5s] [ClientPort:%5s]",getDateAndTime(),SERVER_HOST_NAME,ip,protocol,clientPort);
	}

	fprintf(stderr,"%s",toLog);
	fprintf(serverLog, "%s",toLog);

	if(end)
		closeServerLog();
}

void logClient(char * port, char * fileName, int block, bool end, bool error, char * errorMsg){
	char toLog[LOG_MESSAGE_SIZE];

	if(NULL == clientLog)
		openClientLog(port);

	if(end){
		if(error)
			sprintf(toLog,"\n[%s][File %10s][ERROR: %s]",getDateAndTime(),fileName,errorMsg);
		else
			sprintf(toLog,"\n[%s][File %10s][SUCCED]",getDateAndTime(),fileName);
	}else{
		sprintf(toLog,"\n[%s][File %10s][Send block:%d]",getDateAndTime(),fileName,block);
	}

	fprintf(stderr,"%s",toLog);
	fprintf(clientLog, "%s",toLog);

	if(end)
		closeClientLog();
}
