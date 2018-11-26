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

    pid_t createProcess();
    bool isChild(pid_t pid);
    void redefineSignal(int signal, void(*function)(int));

#endif
