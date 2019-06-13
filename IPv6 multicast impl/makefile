CC = gcc
CFLAGS = 
LIBS = 

PROGS = receiver sender

all: ${PROGS}

receiver: receiver.o
	${CC} ${CFLAGS} -o $@ receiver.o ${LIBS}

sender: sender.o
	${CC} ${CFLAGS} -o $@ sender.o ${LIBS}

clean:
	rm *.o ${PROGS}
