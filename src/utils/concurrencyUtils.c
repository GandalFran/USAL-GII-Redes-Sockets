/*

** Fichero: concurrencyUtils.c
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>

#include "concurrencyUtils.h"

pid_t createProcess(){
  pid_t pid;
  EXIT_ON_FAILURE(-1,pid = fork());
  return pid;
}

bool isChild(pid_t pid){
  return (pid == 0);
}

void redefineSignal(int signal, void(*function)(int)){
  struct sigaction ss;
  memset(&ss,0,sizeof(ss));

  ss.sa_handler=function;
  ss.sa_flags=0;
  EXIT_ON_FAILURE(-1,sigfillset(&ss.sa_mask));

  EXIT_ON_FAILURE(-1,sigaction(signal,&ss,NULL));
}
