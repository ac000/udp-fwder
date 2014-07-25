CC=gcc
CFLAGS=-Wall -g -std=c99 -O2
LDFLAGS=
LIBS=-lrt

client: client.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c nsleep.c ${LIBS}
	$(CC) $(CFLAGS) $(LDFLAGS) -o client client.c nsleep.o ${LIBS}

clean:
	rm -f client nsleep.o
