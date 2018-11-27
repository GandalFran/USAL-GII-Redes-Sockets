CC=gcc
CFLAGS=-g
COFLAGS=-g -c
OBJ= utils/msgUtils.o

#only for olivo
LIBS= -lsocket -lnsl

all: server client removeCompilationWaste

runAll: server client removeCompilationWaste
		./server &
		./client 192.168.70.1 TCP r f1.txt &

runServer: server removeCompilationWaste
		clear
		./server

runClient: client removeCompilationWaste
		clear
		./client 127.20.2.164 TCP r f1.txt
clean:
	rm client
	rm server
	rm peticiones.log
	rm *.txt
	killall -9 server
	echo "HOLA SOY UNA PRUEBA Y OJALA SALGA BIEN " > f1.txt
	clear

removeCompilationWaste:
		rm utils/*.o

server: server.c utils/utils.h $(OBJ)
	$(CC) $(CFLAGS) server.c $(OBJ) -o server
client: client.c utils/utils.h $(OBJ) 
	$(CC) $(CFLAGS) client.c $(OBJ) -o client

msgUtils.o: utils/msgUtils.c utils/msgUtils.h utils/utils.h
	$(CC) $(COFLAGS) utils/msgUtils.c
