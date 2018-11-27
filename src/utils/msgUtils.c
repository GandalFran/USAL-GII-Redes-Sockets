/*

** Fichero: msgUtils.c
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/

#include "utils.h"
#include "msgUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

headers getMessageTypeWithBuffer(char * buffer){
	return buffer[1];
}

rwMsg fillReadMsgWithBuffer(char * buffer){
	rwMsg msg;
	memset(&msg,0,sizeof(rwMsg));

	msg.header = (short) (buffer[1])? READ : WRITE;
	strncpy(msg.fileName,&(buffer[2]),MSG_FILE_NAME_SIZE);
	strncpy(msg.characterMode,&(buffer[2 + MSG_FILE_NAME_SIZE]),MSG_MODE_SIZE);
}
dataMsg fillDataWithBuffer(char * buffer){
	dataMsg msg;
	memset(&msg,0,sizeof(dataMsg));

	msg.header = buffer[1];
	msg.blockNumber = buffer[3];
	strncpy(msg.data,&(buffer[4]),MSG_DATA_SIZE);
}
ackMsg fillAckWithBuffer(char * buffer){
	ackMsg msg;
	memset(&msg,0,sizeof(ackMsg));

	msg.header = buffer[1];
	msg.blockNumber = buffer[3];
}
errMsg fillErrWithBuffer(char * buffer){
	errMsg msg;
	memset(&msg,0,sizeof(errMsg));

	msg.header = buffer[1];
	msg.errorCode = buffer[3];
	strncpy(msg.errorMsg,&(buffer[4]),MSG_ERROR_SIZE);
}


void fillBufferWithReadMsg(rwMsg msg, char * buffer){
	memset(buffer,0,sizeof(buffer));

	buffer[1] = msg.header;
	strncpy(&(buffer[2]),msg.fileName,MSG_FILE_NAME_SIZE);
	strncpy(&(buffer[2 + MSG_FILE_NAME_SIZE]),msg.characterMode,MSG_MODE_SIZE);
}
void fillBufferWithDataMsg(dataMsg msg, char * buffer){
	memset(buffer,0,sizeof(buffer));

	buffer[1] = msg.header;
	buffer[3] = msg.blockNumber;
	strncpy(&(buffer[4]),msg.data,MSG_DATA_SIZE);
}
void fillBufferWithAckMsg(ackMsg msg, char * buffer){
	memset(buffer,0,sizeof(buffer));

	buffer[1] = msg.header;
	buffer[3] = msg.blockNumber;
}
void fillBufferWithErrMsg(errMsg msg, char * buffer){
	memset(buffer,0,sizeof(buffer));

	buffer[1] = msg.header;
	buffer[3] = msg.errorCode;
	strncpy(&(buffer[4]),msg.errorMsg,MSG_ERROR_SIZE);
}