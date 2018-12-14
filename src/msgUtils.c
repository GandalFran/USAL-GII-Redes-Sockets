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
	char header[30];
	
	memset(header,0,30);
	memcpy(&header,buffer,2);

	if(!strcmp(header,READ_HEADER_STR)){
		return READ_TYPE;
	}else if(!strcmp(header,WRITE_HEADER_STR)){
		return WRITE_TYPE;
	}else if(!strcmp(header,DATA_HEADER_STR)){
		return DATA_TYPE;
	}else if(!strcmp(header,ACK_HEADER_STR)){
		return ACK_TYPE;
	}else if(!strcmp(header,ERR_HEADER_STR)){
		return ERR_TYPE;
	}else{
		return UNKNOWN_TYPE;
	}

	return ntohs(header);
}

rwMsg fillReadMsgWithBuffer(char * buffer){
	rwMsg msg;

	memset(&msg,0,sizeof(rwMsg));

	strcpy(msg.fileName,&(buffer[2]));
	strcpy(msg.characterMode,&(buffer[2 + strlen(msg.fileName) + 1]));

	msg.header = getMessageTypeWithBuffer(buffer);

	return msg;
}
dataMsg fillDataWithBuffer(size_t dataSize, char * buffer){
	dataMsg msg;
	uint16_t block;

	memset(&msg,0,sizeof(dataMsg));
	memset(&block,0,sizeof(uint16_t));

	memcpy(&block,&(buffer[2]),sizeof(uint16_t));
	memcpy(msg.data,&(buffer[4]),dataSize-1);

	msg.blockNumber = ntohs(block);
	msg.header = getMessageTypeWithBuffer(buffer);

	return msg;
}
ackMsg fillAckWithBuffer(char * buffer){
	ackMsg msg;
	uint16_t block;

	memset(&msg,0,sizeof(ackMsg));
	memset(&block,0,sizeof(uint16_t));

	memcpy(&block,&(buffer[2]),sizeof(uint16_t));

	msg.blockNumber = ntohs(block);
	msg.header = getMessageTypeWithBuffer(buffer);

	return msg;
}
errMsg fillErrWithBuffer(char * buffer){
	errMsg msg;
	uint16_t ec;

	memset(&msg,0,sizeof(errMsg));
	memset(&ec,0,sizeof(uint16_t));

	memcpy(&ec,&(buffer[2]),sizeof(uint16_t));
	strcpy(msg.errorMsg,&(buffer[4]));

	msg.errorCode = ntohs(ec);
	msg.header = getMessageTypeWithBuffer(buffer);

	return msg;
}

int fillBufferWithReadMsg(bool isRead,char * fileName, char * buffer){
	memset(buffer,0,TAM_BUFFER);

	char mode[] = OCTET_MODE;
	char header[] = ((isRead) ? (READ_HEADER_STR) : (WRITE_HEADER_STR));

	memcpy(buffer,header,2);
	memcpy(&(buffer[2]),fileName,strlen(fileName));
	memcpy(&(buffer[2 + strlen(fileName) + 1]),mode,strlen(mode));

	return (2 + strlen(fileName) + 1 + strlen(mode) + 1);
}
int fillBufferWithDataMsg(int blockNumber, char * data, size_t dataSize,char * buffer){
	uint16_t block;
	memset(buffer,0,TAM_BUFFER);

	char header[] = DATA_HEADER_STR;
	block = htons(blockNumber);

	memcpy(buffer,header,2);
	memcpy(&(buffer[2]),&block,sizeof(uint16_t));
	memcpy(&(buffer[4]),data,dataSize);

	return (2 + 2 + dataSize);
}
int fillBufferWithAckMsg(int blockNumber, char * buffer){
	uint16_t block;
	memset(buffer,0,TAM_BUFFER);

	char header[] = ACK_HEADER_STR;
	block = htons(blockNumber);

	memcpy(buffer,header,2);
	memcpy(&(buffer[2]),&block,sizeof(uint16_t));

	return (2 + 2);
}
int fillBufferWithErrMsg(errorMsgCodes errorcode, char * errorMsg, char * buffer){
	uint16_t ec;
	memset(buffer,0,TAM_BUFFER);

	char header[] = ERR_HEADER_STR;
	ec = htons(errorcode);

	memcpy(buffer,header,2);
	memcpy(&(buffer[2]),&ec,sizeof(uint16_t));
	memcpy(&(buffer[4]),errorMsg,strlen(errorMsg));

	return (2 + 2 + strlen(errorMsg) + 1);
}
