/*

** Fichero: utils.h
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/


#ifndef __UTILS_H
#define __UTILS_H

#include<stdio.h>
#include "logUtils.h"

#define PORT 8455
#define BUFFER_SIZE 1024

#define TRUE 1
#define FALSE 0
typedef unsigned short bool;


#define PRINT_ERROR(returnValue)                                                    \
    do{                                                                             \
        if((returnValue) == -1){                                                    \
            char errorTag[80];                                                      \
            sprintf(errorTag, "\n[%s:%d:%s] ", __FILE__, __LINE__, __FUNCTION__);   \
	    logServer("IP","TCP", -1, TRUE, TRUE, errorTag);		    	    \
            perror(errorTag);                                                       \
        }                                                                           \
    }while(0)

    #define EXIT_ON_FAILURE(errorValue, returnValue) \
    do{                                              \
        if((returnValue) == -1){                     \
	    closeServerLog();			     \
	    closeClientLog();			     \
            PRINT_ERROR(-1);                         \
        }                                            \
    }while(0)

#endif
