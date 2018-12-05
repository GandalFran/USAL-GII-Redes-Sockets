CC=gcc
CFLAGS=-g
COFLAGS=-g -c
OBJ= msgUtils.o
SERVER_IP = 192.168.2.246

#only for olivo
LIBS= -lsocket -lnsl

all: TFTPserver TFTPclient removeCompilationWaste

run: TFTPserver TFTPclient removeCompilationWaste
		./TFTPserver &
		./TFTPclient $(SERVER_IP) UDP w test.mp3 &
runServer: server removeCompilationWaste
		clear
		./server

runClient: TFTPclient removeCompilationWaste
		clear
		./TFTPclient $(SERVER_IP) UDP r bible.txt
clean:
	rm TFTPclient
	rm TFTPserver
	killall -9 TFTPserver
	rm log/*
	rm readedFiles/*
	clear

removeCompilationWaste:
		rm *.o

TFTPserver: TFTPserver.c utils.h $(OBJ)
	$(CC) $(CFLAGS) TFTPserver.c $(OBJ) -o TFTPserver
TFTPclient: TFTPclient.c utils.h $(OBJ) 
	$(CC) $(CFLAGS) TFTPclient.c $(OBJ) -o TFTPclient

msgUtils.o: msgUtils.c msgUtils.h utils.h
	$(CC) $(COFLAGS) msgUtils.c
