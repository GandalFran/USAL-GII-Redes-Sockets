/*

** Fichero: logUtils.h
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/

#ifndef __LOGUTILS_H
#define __LOGUTILS_H

#include "utils.h"

void openServerLog(void);
void openClientLog(char * port);

void closeServerLog(void);
void closeClientLog(void);

void logServer(char * ip, char * protocol, char * clientPort, bool end, bool error, char * errorMsg);
void logClient(char * port, char * fileName, int block, bool end, bool error, char * errorMsg);

#endif