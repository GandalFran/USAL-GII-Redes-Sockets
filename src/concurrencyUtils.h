/*

** Fichero: concurrencyUtils.h
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/

#ifndef __CONCURRENCYUTILS_H
#define __CONCURRENCYUTILS_H

#define PRINT_ERROR(returnValue)                                                    \
    do{                                                                             \
        if((returnValue) == -1){                                                    \
            char errorTag[80];                                                      \
            sprintf(errorTag, "\n[%s:%d:%s] ", __FILE__, __LINE__, __FUNCTION__);   \
            perror(errorTag);                                                       \
        }                                                                           \
    }while(0)

#define EXIT_ON_FAILURE(returnValue)            \
    do{                                         \
        if((returnValue) == -1){                \
            PRINT_ERROR(-1);                    \
        }                                       \
    }while(0)

#define CREATE_PROCESS(value)    EXIT_ON_FAILURE((value) = fork())

#define IF_CHILD(value) if((value) == 0)

#define KILL_PROCESS(pid,signal)                     \
    do{                                              \
        if(pid != 0){                                \
            PRINT_ERROR(kill(pid,signal));           \
            PRINT_ERROR(waitpid(pid,NULL,0));        \
        }                                            \
    }while(0)

#define REDEFINE_SIGNAL(signal,funcion)                         \
    do{                                                         \
        struct sigaction sigactionSs;                           \
        EXIT_ON_FAILURE(sigfillset(&sigactionSs.sa_mask));      \
        sigactionSs.sa_handler=funcion;                         \
        sigactionSs.sa_flags=0;                                 \
        EXIT_ON_FAILURE(sigaction(signal,&sigactionSs,NULL));   \
    }while(0)

#endif