all: server client

static:
	g++ -static -o client client.cpp
	g++ -static -o server server.cpp
revised:
	g++ -D_REVISED_ -o client client.cpp
	g++ -o server server.cpp
staticR:
	g++ -static -D_REVISED_ -o client client.cpp
	g++ -static -o server server.cpp
client: client.cpp
	g++ client.cpp -o client

server: magic server.cpp
	g++ -g -std=c++14 server.cpp magic.o -lpthread -o server
magic: magic.cpp magic.h
	g++ -g -std=c++14 -c magic.cpp -o magic.o