CC=gcc
CFLAGS=-Wall -g -std=c99 -O2
LDFLAGS=
LIBS=-lrt

client: client.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o client client.c ${LIBS}

clean:
	rm -f client
