CC = gcc
CFLAGS = -Wall

all: tcpclient tcpserver updclient updserver

tcpclient: tcpclient.c
    $(CC) $(CFLAGS) -o $@ $^

tcpserver: tcpserver.c
    $(CC) $(CFLAGS) -o $@ $^

updclient: updclient.c
    $(CC) $(CFLAGS) -o $@ $^

updserver: updserver.c
    $(CC) $(CFLAGS) -o $@ $^

clean:
    rm -f tcpclient tcpserver updclient updserver
