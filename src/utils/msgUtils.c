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
	strcpy(msg.fileName,&(buffer[2]));
	strcpy(msg.characterMode,&(buffer[2 + MSG_FILE_NAME_SIZE]));

	return msg;
}
dataMsg fillDataWithBuffer(char * buffer){
	dataMsg msg;
	memset(&msg,0,sizeof(dataMsg));

	msg.header = buffer[1];
	msg.blockNumber = buffer[3];
	strncpy(msg.data,&(buffer[4]),MSG_DATA_SIZE);
	return msg;
}
ackMsg fillAckWithBuffer(char * buffer){
	ackMsg msg;
	memset(&msg,0,sizeof(ackMsg));

	msg.header = buffer[1];
	msg.blockNumber = buffer[3];
	return msg;
}
errMsg fillErrWithBuffer(char * buffer){
	errMsg msg;
	memset(&msg,0,sizeof(errMsg));

	msg.header = buffer[1];
	msg.errorCode = buffer[3];
	strncpy(msg.errorMsg,&(buffer[4]),MSG_ERROR_SIZE);
	return msg;
}


void fillBufferWithReadMsg(bool isRead,char * fileName, char * buffer){
	memset(buffer,0,sizeof(buffer));

	buffer[1] = isRead ? READ : WRITE;
	strncpy(&(buffer[2]),fileName,MSG_FILE_NAME_SIZE);
	strncpy(&(buffer[2 + MSG_FILE_NAME_SIZE-1]),OCTET_MODE,MSG_MODE_SIZE);
}
void fillBufferWithDataMsg(int blockNumber, char * data , char * buffer){
	memset(buffer,0,sizeof(buffer));

	buffer[1] = DATA;
	buffer[3] = blockNumber;
	strncpy(&(buffer[4]),data,MSG_DATA_SIZE);
}
void fillBufferWithAckMsg(int blockNumber, char * buffer){
	memset(buffer,0,sizeof(buffer));

	buffer[1] = ACK;
	buffer[3] = blockNumber;
}
void fillBufferWithErrMsg(errorMsgCodes errorcode, char * errorMsg, char * buffer){
	memset(buffer,0,sizeof(buffer));

	buffer[1] = ERR;
	buffer[3] = errorcode;
	strncpy(&(buffer[4]),errorMsg,MSG_ERROR_SIZE);
}