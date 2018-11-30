CC=gcc
CFLAGS=-g
COFLAGS=-g -c
OBJ= utils/msgUtils.o
SERVER_IP = 192.168.70.1

#only for olivo
LIBS= -lsocket -lnsl

all: server client removeCompilationWaste

run: server client removeCompilationWaste
		./server &
		./client $(SERVER_IP) TCP r test.mp4 &
		./client $(SERVER_IP) TCP w test.mp3 &
		./client $(SERVER_IP) TCP r test.mid &
		./client $(SERVER_IP) TCP w bible.txt &

runServer: server removeCompilationWaste
		clear
		./server

runClient: client removeCompilationWaste
		clear
		./client $(SERVER_IP) TCP r bible.txt
clean:
	rm client
	rm server
	rm log/*
	killall -9 server
	clear

removeCompilationWaste:
		rm utils/*.o

server: server.c utils/utils.h $(OBJ)
	$(CC) $(CFLAGS) server.c $(OBJ) -o server
client: client.c utils/utils.h $(OBJ) 
	$(CC) $(CFLAGS) client.c $(OBJ) -o client

msgUtils.o: utils/msgUtils.c utils/msgUtils.h utils/utils.h
	$(CC) $(COFLAGS) utils/msgUtils.c
