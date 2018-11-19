
server: server.c concurrencyUtils.o connectionUtils.o logUtils.o
	gcc -g server.c concurrencyUtils.o connectionUtils.o logUtils.o -o server
client: client.c concurrencyUtils.o connectionUtils.o logUtils.o
	gcc -g server.c concurrencyUtils.o connectionUtils.o logUtils.o -o client

concurrencyUtils.o: concurrencyUtils.h concurrencyUtils.c 
	gcc -g -c concurrencyUtils.c
connectionUtils.o: connectionUtils.c connectionUtils.h
	gcc -c connectionUtils.c
logUtils.o : logUtils.c logUtils.h
	gcc -c logUtils.c


clean:
	rm *.o
