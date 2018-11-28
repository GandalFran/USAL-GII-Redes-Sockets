/*

** Fichero: msgUtils.h
** Autores:
** Francisco Pinto Santos  DNI 70918455W
** Hector Sanchez San Blas DNI 70901148Z
*/

#ifndef __MSGUTILS_H
#define __MSGUTILS_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "utils.h"

#define DATA_SIZE(size) ( (size) - 4)

//TODO mirar cuales son los tamanos de estos y el codigo que corresopnde a octet
#define MSG_FILE_NAME_SIZE 128
#define MSG_MODE_SIZE 128
#define MSG_DATA_SIZE 512
#define MSG_ERROR_SIZE 512

#define OCTET_MODE "Octet"

typedef char byte;

typedef enum { READ_TYPE=1,WRITE_TYPE=2,DATA_TYPE=3,ACK_TYPE=4,ERR_TYPE=5} headers;
typedef enum { UNKNOWN=0, FILE_NOT_FOUND=1, DISK_FULL=3, ILLEGAL_OPERATION=4, FILE_ALREADY_EXISTS=6 } errorMsgCodes;

typedef struct{
	 short header;
	 byte fileName[MSG_FILE_NAME_SIZE];
	 byte separator;
	 byte characterMode[MSG_MODE_SIZE];
	 byte tail;
}rwMsg;

typedef struct{
	short header;
	short blockNumber;
	byte data[MSG_DATA_SIZE];
}dataMsg;

typedef struct{
	short header;
	short blockNumber;
}ackMsg;

typedef struct{
	short header;
	short errorCode;
	byte errorMsg[MSG_ERROR_SIZE];
	byte tail;
}errMsg;

headers getMessageTypeWithBuffer(char * buffer);


rwMsg fillReadMsgWithBuffer(char * buffer);
dataMsg fillDataWithBuffer(size_t dataSize, char * buffer);
ackMsg fillAckWithBuffer(char * buffer);
errMsg fillErrWithBuffer(char * buffer);

int fillBufferWithReadMsg(bool isRead,char * fileName, char * buffer);
int fillBufferWithDataMsg(int blockNumber, char * data, size_t dataSize , char * buffer);
int fillBufferWithAckMsg(int blockNumber, char * buffer);
int fillBufferWithErrMsg(errorMsgCodes errorcode, char * errorMsg, char * buffer);

#endif
