CC=gcc
CFLAGS=-Wall -g -std=c99 -O2 -Wp,-D_FORTIFY_SOURCE=2 -D_FILE_OFFSET_BITS=64 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -fPIE
LDFLAGS=-Wl,-z,now -pie
LIBS=-lcurl -pthread -lrt

udp-fwder: udp-server.c
	 $(CC) $(CFLAGS) $(LDFLAGS) -o udp-server udp-server.c ${LIBS}
