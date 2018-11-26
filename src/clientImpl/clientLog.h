
/*

** Fichero: clientLog.h
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/

#ifndef __CLIENTLOG_H
#define __CLIENTLOG_H

#include "../utils/utils.h"

typedef unsigned short bool;

#define EXIT_ON_WRONG_VALUE(wrongValue, errorMsg, returnValue)                          \
do{                                              		    	                              \
    if((returnValue) == (wrongValue)){		    	                                        \
        char errorTag[200];                                                             \
        fprintf(stderr, "\n[%s:%d:%s]%s", __FILE__, __LINE__, __FUNCTION__,errorMsg);   \
        exit(EXIT_SUCCESS);		    	                                                    \
        exit(0);                                                                        \
    }		    	                                                                          \
}while(0)

void logError(int port, char * errorMsg);
void logc(int port, char * fileName, int block, bool end);

#endif
