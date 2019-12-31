all: server client

static: sserver sclient

client: client.cpp
	g++ -g -std=c++14 -D_REVISED_ -o client client.cpp
server: magic server.cpp
	g++ -g -std=c++14 server.cpp magic.o -lpthread -o server

sserver: smagic server.cpp
	g++ -g -std=c++14  server.cpp  smagic.o -lpthread -o server 
sclient: client.cpp
	g++ -g -std=c++14 -static -D_REVISED_ -o client client.cpp

smagic: magic.cpp magic.h
	g++ -g -std=c++14 -static -c magic.cpp -o smagic.o
magic: magic.cpp magic.h
	g++ -g -std=c++14 -c magic.cpp -o magic.o

clean:
	rm -f *.o server client