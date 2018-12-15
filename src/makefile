CC=gcc
CFLAGS=-g
COFLAGS=-g -c
OBJ= msgUtils.o
LIBS=-lsocket -lnsl

all: TFTPserver TFTPclient removeCompilationWaste
	
TFTPserver: TFTPserver.c utils.h $(OBJ)
	$(CC) $(CFLAGS) TFTPserver.c $(OBJ) $(LIBS) -o TFTPserver
TFTPclient: TFTPclient.c utils.h $(OBJ) 
	$(CC) $(CFLAGS) TFTPclient.c $(OBJ) $(LIBS) -o TFTPclient
msgUtils.o: msgUtils.c msgUtils.h utils.h
	$(CC) $(COFLAGS) msgUtils.c

removeCompilationWaste:
	rm *.o
