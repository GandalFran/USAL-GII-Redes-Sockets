/*

** Fichero: server.c
** Autores:s
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include "utils.h"
#include "logUtils.h"
#include "msgUtils.h"
#include "concurrencyUtils.h"

void SIGTERMHandler(int);
void SIGALRMHandler(int);
void tcpServer();
void udpServer();

int main(int argc, char * argv[]){
  //Block all signals and redefine sigterm
    sigset_t signalSet;
    EXIT_ON_FAILURE(sigfillset(&signalSet));
    EXIT_ON_FAILURE(sigprocmask(SIG_SETMASK, &signalSet, NULL));
    redefineSignal(SIGTERM,SIGTERMHandler);

  //Make bifurcation of the process, and unlock signals
    pid_t pid;

    pid = createProcess();
    if(isChild(pid)){ //udp server
      redefineSignal(SIGALRM, SIGALRMHandler);
      EXIT_ON_FAILURE(sigdelset(&signalSet,SIGTERM));
      EXIT_ON_FAILURE(sigdelset(&signalSet,SIGALRM));
      EXIT_ON_FAILURE(sigprocmask(SIG_SETMASK, &signalSet, NULL));
      udpServer();
    }else{  //tcp server
      EXIT_ON_FAILURE(sigdelset(&signalSet,SIGTERM));
      EXIT_ON_FAILURE(sigprocmask(SIG_SETMASK, &signalSet, NULL));
      tcpServer();
    }
}

void SIGTERMHandler(int ss){
  closeServerLog();
  exit(EXIT_SUCCESS);
}

void SIGALRMHandler(int ss){

}

void tcpServer(){

  while(TRUE){

  }
}

void udpServer(){

  while(TRUE){

  }
}
