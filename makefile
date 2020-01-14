all: server client

static: sserver sclient

client: client.cpp
	g++ -g -std=c++14  client.cpp -lpthread -l ssl -l crypto -o client
server: magic server.cpp
	g++ -g -std=c++14 server.cpp magic.o -lpthread -l ssl -l crypto -o server

sserver: smagic server.cpp
	g++ -g -std=c++14  server.cpp  smagic.o -lpthread -l ssl -l crypto -o server 
sclient: client.cpp
	g++ -g -std=c++14 client.cpp -lpthread -l ssl -l crypto -o client

smagic: magic.cpp magic.h
	g++ -g -std=c++14 -static -c magic.cpp -o smagic.o
magic: magic.cpp magic.h
	g++ -g -std=c++14 -c magic.cpp -o magic.o

clean:
	rm -f *.o server client