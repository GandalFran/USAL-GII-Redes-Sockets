CC=gcc
CFLAGS=-g
COFLAGS=-g -c
OBJ= msgUtils.o
LIBS=-lsocket -lnsl

all: servidor cliente removeCompilationWaste
	
servidor: TFTPserver.c utils.h $(OBJ)
	$(CC) $(CFLAGS) TFTPserver.c $(OBJ) $(LIBS) -o servidor
cliente: TFTPclient.c utils.h $(OBJ) 
	$(CC) $(CFLAGS) TFTPclient.c $(OBJ) $(LIBS) -o cliente
	
msgUtils.o: msgUtils.c msgUtils.h utils.h
	$(CC) $(COFLAGS) msgUtils.c

removeCompilationWaste:
	rm *.o
