/*

** Fichero: concurrencyUtils.h
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/

#ifndef __CONCURRENCYUTILS_H
#define __CONCURRENCYUTILS_H

#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

  #define PRINT_ERROR(returnValue)                                                  \
    do{                                                                             \
        if((returnValue) == -1){                                                    \
            char errorTag[80];                                                      \
            sprintf(errorTag, "\n[%s:%d:%s] ", __FILE__, __LINE__, __FUNCTION__);   \
            perror(errorTag);                                                       \
        }                                                                           \
    }while(0)

    #define EXIT_ON_FAILURE(returnValue)        \
    do{                                         \
        if((returnValue) == -1){                \
            PRINT_ERROR(-1);                    \
        }                                       \
    }while(0)

    pid_t createProcess();
    bool isChild(pid_t pid);
    void redefineSignal(int signal, void(*function)(int));

#endif
