/*

** Fichero: msgUtils.c
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/
#
#include "msgUtils.h"

void fillReadMsg(rwMsg*msg, opMode mode, char * fileName){
	memset(msg,0,sizeof(rwMsg));
	msg->operationMode = (byte) mode;
	strcpy(msg->fileName,fileName);
	strcpy(msg->characterMode,OCTET_MODE);
}

void fillDataMsg(dataMsg*msg, short blockNumber, char * data){
	memset(msg,0,sizeof(dataMsg));
	msg->blockNumber = (short) blockNumber;
	strcpy(msg->data,data);
}
void fillAckMsg(ackMsg*msg, short blockNumber){
	memset(msg,0,sizeof(ackMsg));
	msg->blockNumber = (short) blockNumber;
}

void fillErrMsg(errMsg*msg, errorMsgCodes errorCode, char * errorMsg){
	memset(msg,0,sizeof(errMsg));
	msg->errorCode = (short) errorCode;
	strcpy(msg->errorMsg,errorMsg);
}
