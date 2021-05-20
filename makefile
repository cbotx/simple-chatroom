TARGET = wserver
CC = g++
CFLAGS = -g -Wall -pthread

all : wserver wclient

wserver : wserver.o tcp_server.o
	$(CC) $(CFLAGS) -o wserver wserver.o tcp_server.o
	
wclient : wclient.o tcp_client.o
	$(CC) $(CFLAGS) -o wclient wclient.o tcp_client.o

wserver.o : wserver.cc networking/tcp_server.h
	$(CC) $(CFLAGS) -c wserver.cc

wsclient.o : wsclient.cc networking/tcp_sclient.h
	$(CC) $(CFLAGS) -c wsclient.cc

tcp_server.o : networking/tcp_server.cc networking/tcp_server.h networking/common.h
	$(CC) $(CFLAGS) -c networking/tcp_server.cc

tcp_client.o : networking/tcp_client.cc networking/tcp_client.h networking/common.h
	$(CC) $(CFLAGS) -c networking/tcp_client.cc

clean:
	$(RM) wserver wclient
	