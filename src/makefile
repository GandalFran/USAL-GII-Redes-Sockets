CC=gcc
CFLAGS=-g
COFLAGS=-g -c
OBJ= utils/msgUtils.o
CLIENTS = clientImpl/udpClient.o clientImpl/tcpClient.o clientImpl/clientLog.o

#only for olivo
LIBS= -lsocket -lnsl

all: server client removeCompilationWaste

runAll: server client removeCompilationWaste
		./server &
		./client 192.168.70.1 TCP r f1.txt &

runServer: server
		rm utils/*.o
		clear
		./server

runClient: client
		rm utils/*.o
		rm clientImpl/*.o
		clear
		./client 127.20.2.164 TCP r f1
clean:
	rm client
	rm server
	killall -9 server
	killall -9 client
	clear

removeCompilationWaste:
		rm utils/*.o
		rm clientImpl/*.o

server: serverImpl/server.c utils/utils.h $(OBJ)
	$(CC) $(CFLAGS) serverImpl/server.c $(OBJ) -o server
client: clientImpl/client.c utils/utils.h $(OBJ) $(CLIENTS)
	$(CC) $(CFLAGS) clientImpl/client.c $(OBJ) $(CLIENTS) -o client

tcpClient.o: clientImpl/tcpClient.h clientImpl/tcpClient.c utils/utils.h
	$(CC) $(COFLAGS) clientImpl/tcpClient.c
udpClient.o: clientImpl/udpClient.h clientImpl/udpClient.c utils/utils.h
	$(CC) $(COFLAGS) clientImpl/udpClient.c
clientLog.o: clientImpl/clientLog.h clientImpl/clientLog.c utils/utils.h
	$(CC) $(COFLAGS) clientImpl/clientLog.c

msgUtils.o: utils/msgUtils.c utils/msgUtils.h utils/utils.h
	$(CC) $(COFLAGS) utils/msgUtils.c
