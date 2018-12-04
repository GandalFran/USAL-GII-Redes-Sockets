CC=gcc
CFLAGS=-g
COFLAGS=-g -c
OBJ= msgUtils.o
SERVER_IP = 192.168.70.1

#only for olivo
LIBS= -lsocket -lnsl

all: server TFTPclient removeCompilationWaste

run: server TFTPclient removeCompilationWaste
		./server &
		./TFTPclient $(SERVER_IP) TCP r test.mp3 &
		./TFTPclient $(SERVER_IP) UDP r test1.mp4 &
		./TFTPclient $(SERVER_IP) TCP w test.mp3 &
		./TFTPclient $(SERVER_IP) UDP w test1.mp4 &

runServer: server removeCompilationWaste
		clear
		./server

runClient: TFTPclient removeCompilationWaste
		clear
		./TFTPclient $(SERVER_IP) UDP r bible.txt
clean:
	rm TFTPclient
	rm server
	killall -9 server
	rm log/*
	rm readedFiles/*
	clear

removeCompilationWaste:
		rm *.o

server: server.c utils.h $(OBJ)
	$(CC) $(CFLAGS) server.c $(OBJ) -o server
TFTPclient: TFTPclient.c utils.h $(OBJ) 
	$(CC) $(CFLAGS) TFTPclient.c $(OBJ) -o TFTPclient

msgUtils.o: msgUtils.c msgUtils.h utils.h
	$(CC) $(COFLAGS) msgUtils.c
