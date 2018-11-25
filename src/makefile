CC=gcc
CFLAGS=-g
COFLAGS=-g -c
OBJ= concurrencyUtils.o msgUtils.o logUtils.o

all: server client
		rm *.o
		rm *.out

server: server.c utils.h $(OBJ)
	$(CC) $(CFLAGS) server.c $(OBJ) -o server
client: client.c utils.h $(OBJ)
	$(CC) $(CFLAGS) client.c $(OBJ) -o client

concurrencyUtils.o: concurrencyUtils.h concurrencyUtils.c utils.h
	$(CC) $(COFLAGS) concurrencyUtils.c
msgUtils.o: msgUtils.c msgUtils.h utils.h
	$(CC) $(COFLAGS) msgUtils.c
logUtils.o: logUtils.c logUtils.h utils.h
	$(CC) $(COFLAGS) logUtils.c

clean:
		rm *.o
