

#ifndef __MSGUTILS_H
#define __MSGUTILS_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include "utils.h"

#define DATA_SIZE(size) ((size) - 4)

#define MSG_FILE_NAME_SIZE 30
#define MSG_MODE_SIZE 128
#define MSG_DATA_SIZE 512
#define MSG_ERROR_SIZE 128
#define TIME_OUT 10

#define OCTET_MODE "octet"

#define READ_HEADER_STR  "01"
#define WRITE_HEADER_STR "02"
#define DATA_HEADER_STR  "03"
#define ACK_HEADER_STR   "04"
#define ERR_HEADER_STR   "05"

typedef enum { READ_TYPE=1,WRITE_TYPE=2,DATA_TYPE=3,ACK_TYPE=4,ERR_TYPE=5,UNKNOWN_TYPE=6} headers;
typedef enum { UNKNOWN=0, FILE_NOT_FOUND=1, DISK_FULL=3, ILLEGAL_OPERATION=4, FILE_ALREADY_EXISTS=6 } errorMsgCodes;

typedef struct{
	 uint16_t header;
	 uint8_t fileName[MSG_FILE_NAME_SIZE];
	 uint8_t characterMode[MSG_MODE_SIZE];
}rwMsg;

typedef struct{
	uint16_t header;
	uint16_t blockNumber;
	uint8_t data[MSG_DATA_SIZE];
}dataMsg;

typedef struct{
	uint16_t header;
	uint16_t blockNumber;
}ackMsg;

typedef struct{
	uint16_t header;
	uint16_t errorCode;
	uint8_t errorMsg[MSG_ERROR_SIZE];
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

int check_timeout(int s, char *buffer, struct sockaddr_in clientaddr_in, socklen_t addrlen);

#endif
