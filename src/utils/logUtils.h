/*

** Fichero: logUtils.h
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/

#ifndef __LOGUTILS_H
#define __LOGUTILS_H

#include "utils.h"

typedef unsigned short bool;

void openServerLog(void);
void openClientLog(int port);

void closeServerLog(void);
void closeClientLog(void);

void logServer(char * ip, char * protocol, int clientPort, bool end, bool error, char * errorMsg);
void logClient(int port, char * fileName, int block, bool end, bool error, char * errorMsg);

#endif
