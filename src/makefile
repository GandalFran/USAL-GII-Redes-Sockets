all: server client

server: server.c utils.h concurrencyUtils.o connectionUtils.o logUtils.o
	gcc -g server.c concurrencyUtils.o connectionUtils.o logUtils.o -o server
client: client.c utils.h concurrencyUtils.o connectionUtils.o logUtils.o
	gcc -g server.c concurrencyUtils.o connectionUtils.o logUtils.o -o client

concurrencyUtils.o: concurrencyUtils.h concurrencyUtils.c utils.h
	gcc -g -c concurrencyUtils.c
connectionUtils.o: connectionUtils.c connectionUtils.h utils.h
	gcc -c connectionUtils.c
logUtils.o : logUtils.c logUtils.h utils.h
	gcc -c logUtils.c

clean:
	rm *.o
