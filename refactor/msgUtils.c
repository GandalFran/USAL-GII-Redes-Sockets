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
	uint16_t header;
	memcpy(&header,buffer,sizeof(uint16_t));
	return ntohs(header);
}


rwMsg fillReadMsgWithBuffer(char * buffer){
	rwMsg msg;
	uint16_t header;

	memset(&msg,0,sizeof(rwMsg));
	memset(&header,0,sizeof(uint16_t));

	memcpy(&header,buffer,sizeof(uint16_t));
	strcpy(msg.fileName,&(buffer[2]));
	strcpy(msg.characterMode,&(buffer[2 + strlen(msg.fileName) + 1]));

	msg.header = ntohs(header);

	return msg;
}
dataMsg fillDataWithBuffer(size_t dataSize, char * buffer){
	dataMsg msg;
	uint16_t header, block;

	memset(&msg,0,sizeof(dataMsg));
	memset(&block,0,sizeof(uint16_t));
	memset(&header,0,sizeof(uint16_t));

	memcpy(&header,buffer,sizeof(uint16_t));
	memcpy(&block,&(buffer[2]),sizeof(uint16_t));
	memcpy(msg.data,&(buffer[4]),dataSize-1);

	msg.header = ntohs(header);
	msg.blockNumber = ntohs(block);

	return msg;
}
ackMsg fillAckWithBuffer(char * buffer){
	ackMsg msg;
	uint16_t header, block;

	memset(&msg,0,sizeof(ackMsg));
	memset(&block,0,sizeof(uint16_t));
	memset(&header,0,sizeof(uint16_t));

	memcpy(&header,buffer,sizeof(uint16_t));
	memcpy(&block,&(buffer[2]),sizeof(uint16_t));

	msg.header = ntohs(header);
	msg.blockNumber = ntohs(block);

	return msg;
}
errMsg fillErrWithBuffer(char * buffer){
	errMsg msg;
	uint16_t header, ec;

	memset(&msg,0,sizeof(errMsg));
	memset(&ec,0,sizeof(uint16_t));
	memset(&header,0,sizeof(uint16_t));

	memcpy(&header,buffer,sizeof(uint16_t));
	memcpy(&ec,&(buffer[2]),sizeof(uint16_t));
	strcpy(msg.errorMsg,&(buffer[4]));

	msg.header = ntohs(header);
	msg.errorCode = ntohs(ec);

	return msg;
}


int fillBufferWithReadMsg(bool isRead,char * fileName, char * buffer){
	uint16_t header;
	char mode[] = OCTET_MODE;
	memset(buffer,0,TAM_BUFFER);

	header = htons(isRead ? 1 : 2);

	memcpy(buffer,&header,sizeof(uint16_t));
	memcpy(&(buffer[2]),fileName,strlen(fileName));
	memcpy(&(buffer[2 + strlen(fileName) + 1]),mode,strlen(mode));

	return (2 + strlen(fileName) + 1 + strlen(mode) + 1);
}
int fillBufferWithDataMsg(int blockNumber, char * data, size_t dataSize,char * buffer){
	uint16_t header, block;
	memset(buffer,0,TAM_BUFFER);

	header = htons(3);
	block = htons(blockNumber);

	memcpy(buffer,&header,sizeof(uint16_t));
	memcpy(&(buffer[2]),&block,sizeof(uint16_t));
	memcpy(&(buffer[4]),data,dataSize);

	return (2 + 2 + dataSize);
}
int fillBufferWithAckMsg(int blockNumber, char * buffer){
	uint16_t header, block;
	memset(buffer,0,TAM_BUFFER);

	header = htons(4);
	block = htons(blockNumber);

	memcpy(buffer,&header,sizeof(uint16_t));
	memcpy(&(buffer[2]),&block,sizeof(uint16_t));

	return (2 + 2);
}
int fillBufferWithErrMsg(errorMsgCodes errorcode, char * errorMsg, char * buffer){
	uint16_t header, ec;
	memset(buffer,0,TAM_BUFFER);

	header = htons(5);
	ec = htons(errorcode);

	memcpy(buffer,&header,sizeof(uint16_t));
	memcpy(&(buffer[2]),&ec,sizeof(uint16_t));
	memcpy(&(buffer[4]),errorMsg,strlen(errorMsg));

	return (2 + 2 + strlen(errorMsg) + 1);
}
