/*

** Fichero: utils.h
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/


#ifndef __UTILS_H
#define __UTILS_H

#include<stdio.h>

#define READ_FILES_FOLDER "readedFiles"


#define TIME_STRING_SIZE 100
#define LOG_MESSAGE_SIZE 1000

#define LOG_FOLDER "log"
#define SERVER_LOG "peticiones.log"
#define CLIENT_LOG_EXT ".txt"

#define LOG_START_READ 0 
#define LOG_START_WRITE 1 
#define LOG_READ 2
#define LOG_WRITE 3 
#define LOG_END 4
#define LOG_END_READ 5
#define LOG_END_WRITE 6

typedef enum {TCP_MODE, UDP_MODE}ProtocolMode;
#define MODE_STR(mode) ( ((mode) == TCP_MODE) ? ("TCP"):("UDP") ) 


#define PORT 8456
#define TIMEOUT 1000

#define TAM_BUFFER 1024

#define TRUE 1
#define FALSE 0
typedef unsigned short bool;

#endif
