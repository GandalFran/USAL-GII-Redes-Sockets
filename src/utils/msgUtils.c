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

	msg.header = (short) (buffer[1])? READ_TYPE : WRITE_TYPE;
	strcpy(msg.fileName,&(buffer[2]));
	strcpy(msg.characterMode,&(buffer[2 + MSG_FILE_NAME_SIZE-1]));

	return msg;
}
dataMsg fillDataWithBuffer(size_t dataSize, char * buffer){
	dataMsg msg;
	memset(&msg,0,sizeof(dataMsg));

	msg.header = buffer[1];
	msg.blockNumber = buffer[3];
	strcpy(msg.data,&(buffer[4]));

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
	strcpy(msg.errorMsg,&(buffer[4]));
	return msg;
}


int fillBufferWithReadMsg(bool isRead,char * fileName, char * buffer){
	memset(buffer,0,sizeof(buffer));

	buffer[1] = isRead ? READ_TYPE : WRITE_TYPE;
	strcpy(&(buffer[2]),fileName);
	strcpy(&(buffer[2 + MSG_FILE_NAME_SIZE-1]),OCTET_MODE);

	return sizeof(rwMsg);
}
int fillBufferWithDataMsg(int blockNumber, char * data, size_t dataSize,char * buffer){
	memset(buffer,0,sizeof(buffer));

	buffer[1] = DATA_TYPE;
	buffer[3] = blockNumber;
	//strncpy(&(buffer[4]),data, dataSize-4);
	strcpy(&(buffer[4]),data);

	return DATA_SIZE(dataSize);
}
int fillBufferWithAckMsg(int blockNumber, char * buffer){
	memset(buffer,0,sizeof(buffer));

	buffer[1] = ACK_TYPE;
	buffer[3] = blockNumber;

	return sizeof(ackMsg);
}
int fillBufferWithErrMsg(errorMsgCodes errorcode, char * errorMsg, char * buffer){
	memset(buffer,0,sizeof(buffer));

	buffer[1] = ERR_TYPE;
	buffer[3] = errorcode;
	strcpy(&(buffer[4]),errorMsg);

	return sizeof(errMsg);
}