CC = gcc
CFLAGS = -Wall -pthread

all: server client

server: server.c
	$(CC) $(CFLAGS) server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f server client *.o
	rm -rf server_files
	mkdir -p server_files
