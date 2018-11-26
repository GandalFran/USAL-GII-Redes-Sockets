
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "clientLog.h"

#define CLIENT_FILE_PATH_SIZE 20

const char * getDateAndTime(){
	static char timeString[TIME_STRING_SIZE];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(timeString,"%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	return timeString;
}

void logc(int port, char * fileName, int block, bool end){
  char toLog[LOG_MESSAGE_SIZE];
  char path[CLIENT_FILE_PATH_SIZE];

  sprintf(path,"%d.txt",port);
  FILE*logFile = fopen(path,"a+");

	if(end){
			sprintf(toLog,"\n[%s][File %10s][SUCCED]",getDateAndTime(),fileName);
	}else{
		  sprintf(toLog,"\n[%s][File %10s][Send block:%d]",getDateAndTime(),fileName,block);
	}

	fprintf(stderr,"%s",toLog);
	fprintf(logFile, "%s",toLog);

	fclose(logFile);
}

void logError(int port, char * errorMsg){
	char toLog[LOG_MESSAGE_SIZE];
  char path[CLIENT_FILE_PATH_SIZE];

  sprintf(path,"%d.txt",port);
  FILE*logFile = fopen(path,"a+");

	sprintf(toLog,"\n[%s][ERROR: %s]",getDateAndTime(),errorMsg);
	fprintf(stderr,"%s",toLog);
	fprintf(logFile, "%s",toLog);

	fclose(logFile);
}
