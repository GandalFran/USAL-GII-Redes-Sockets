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
	return header;
}


rwMsg fillReadMsgWithBuffer(char * buffer){
	rwMsg msg;
	memset(&msg,0,sizeof(rwMsg));

	memcpy(&(msg.header),buffer,sizeof(uint16_t));
	strcpy(msg.fileName,&(buffer[2]));
	strcpy(msg.characterMode,&(buffer[2 + strlen(msg.fileName) + 1]));

	return msg;
}
dataMsg fillDataWithBuffer(size_t dataSize, char * buffer){
	dataMsg msg;
	memset(&msg,0,sizeof(dataMsg));

	memcpy(&(msg.header),buffer,sizeof(uint16_t));
	memcpy(&(msg.blockNumber),&(buffer[2]),sizeof(uint16_t));
	memcpy(msg.data,&(buffer[4]),dataSize-1);

	return msg;
}
ackMsg fillAckWithBuffer(char * buffer){
	ackMsg msg;
	memset(&msg,0,sizeof(ackMsg));

	memcpy(&(msg.header),buffer,sizeof(uint16_t));
	memcpy(&(msg.blockNumber),&(buffer[2]),sizeof(uint16_t));

	return msg;
}
errMsg fillErrWithBuffer(char * buffer){
	errMsg msg;
	memset(&msg,0,sizeof(errMsg));

	memcpy(&(msg.header),buffer,sizeof(uint16_t));
	memcpy(&(msg.errorCode),&(buffer[2]),sizeof(uint16_t));
	strcpy(msg.errorMsg,&(buffer[4]));
	return msg;
}


int fillBufferWithReadMsg(bool isRead,char * fileName, char * buffer){
	memset(buffer,0,sizeof(buffer));

	uint16_t header = isRead ? READ_TYPE : WRITE_TYPE;
	char mode[] = OCTET_MODE;

	memcpy(buffer,&header,sizeof(uint16_t));
	memcpy(&(buffer[2]),fileName,strlen(fileName));
	memcpy(&(buffer[2 + strlen(fileName) + 1]),mode,strlen(mode));

	return (4 + sizeof(fileName) + sizeof(mode));
}
int fillBufferWithDataMsg(int blockNumber, char * data, size_t dataSize,char * buffer){
	memset(buffer,0,sizeof(buffer));

	uint16_t header = DATA_TYPE;
	memcpy(buffer,&header,sizeof(uint16_t));
	memcpy(&(buffer[2]),&blockNumber,sizeof(uint16_t));
	memcpy(&(buffer[4]),data,dataSize);

	return 5 + dataSize;
}
int fillBufferWithAckMsg(int blockNumber, char * buffer){
	memset(buffer,0,sizeof(buffer));

	uint16_t header = ACK_TYPE;
	memcpy(buffer,&header,sizeof(uint16_t));
	memcpy(&(buffer[2]),&blockNumber,sizeof(uint16_t));

	return 4;
}
int fillBufferWithErrMsg(errorMsgCodes errorcode, char * errorMsg, char * buffer){
	memset(buffer,0,sizeof(buffer));

	uint16_t header = ERR_TYPE;
	memcpy(buffer,&header,sizeof(uint16_t));
	memcpy(&(buffer[2]),&errorcode,sizeof(uint16_t));
	memcpy(&(buffer[4]),errorMsg,strlen(errorMsg));

	return 5 + sizeof(errorMsg);
}

int check_timeout(int s, char *buffer, struct sockaddr_in clientaddr_in, socklen_t addrlen){
    fd_set f;
    int value;
    struct timeval time;
    
    // set up the file descriptor set
    FD_ZERO(&f);
    FD_SET(s, &f);
    
    // set up the struct timeval for the timeout
    time.tv_sec = TIME_OUT;
    time.tv_usec = 0;
    
    // wait until timeout or data received
    value = select(s+1, &f, NULL, NULL, &time); //Sirve para esperar hasta que haya algo que recibir
    if (value == 0){
        return -2; // timeout!
    } else if (value == -1){
        return -1; // error
    }
    
    return recvfrom(s,buffer,TAM_BUFFER,0,(struct sockaddr *)&clientaddr_in,&addrlen);
}
