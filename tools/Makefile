CC=gcc
CFLAGS=-Wall -Wextra -g -std=c99 -O2
LDFLAGS=
LIBS=-lrt

client: client.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c nsleep.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o client client.c nsleep.o ${LIBS}

clean:
	rm -f client nsleep.o
