
#ifndef __UTILS_H
#define __UTILS_H

#include<stdio.h>

typedef unsigned short bool;
typedef enum {TCP_MODE, UDP_MODE}ProtocolMode;
typedef enum{LOG_START_READ, LOG_START_WRITE, LOG_READ, LOG_WRITE, LOG_END, LOG_END_READ, LOG_END_WRITE} LogOptions;

#define TRUE 1
#define FALSE 0

#define TIME_STRING_SIZE 100
#define LOG_MESSAGE_SIZE 1000

#define SERVER_FILES_FOLDER "ficherosTFTPserver"
#define CLIENT_FILES_FOLDER "ficherosTFTPclient"
#define SERVER_LOG "peticiones.log"
#define CLIENT_LOG_EXT ".txt"

#define MODE_STR(mode) ( ((mode) == TCP_MODE) ? ("TCP"):("UDP") ) 

#define PORT 8455
#define TIMEOUT 10
#define RETRIES 3

#define TAM_BUFFER 1024

#endif
