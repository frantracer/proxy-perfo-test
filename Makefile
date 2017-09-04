CC=g++
CCFLAGS=-std=c++11

all: client server

client: client.cpp
	$(CC) $(CCFLAGS) -o client client.cpp

server: server.cpp
	$(CC) $(CCFLAGS) -o server server.cpp
 
clean:
	rm -rf client server *.o

