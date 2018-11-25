CC=gcc
CFLAGS=-g
COFLAGS=-g -c
OBJ= utils/concurrencyUtils.o utils/msgUtils.o utils/logUtils.o
CLIENTS = clientImpl/udpClient.o clientImpl/tcpClient.o

all: server client removeCompilationWaste

runServer: server
		rm utils/*.o
		clear
		echo "aqui va el comando para ejecutar"
runClient: client
		rm utils/*.o
		rm clientImpl/*.o
		clear
		echo "aqui va el comando para ejecutar"

clean:
	rm client
	rm server
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

concurrencyUtils.o: utils/concurrencyUtils.h utils/concurrencyUtils.c utils/utils.h
	$(CC) $(COFLAGS) utils/concurrencyUtils.c
msgUtils.o: utils/msgUtils.c utils/msgUtils.h utils/utils.h
	$(CC) $(COFLAGS) utils/msgUtils.c
logUtils.o: utils/logUtils.c utils/logUtils.h utils/utils.h
	$(CC) $(COFLAGS) utils/logUtils.c
