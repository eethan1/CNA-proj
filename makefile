all: server client

static: sserver sclient

client: client.cpp
	g++ -g -std=c++14 -D_REVISED_ -o client client.cpp
server: magic server.cpp
	g++ -g -std=c++14 server.cpp magic.o -lpthread -o server

sserver: magic server.cpp
	g++ -g -std=c++14 -lpthread -static server.cpp  magic.o -o server 
sclient: client.cpp
	g++ -g -static -std=c++14 -D_REVISED_ -o client client.cpp

magic: magic.cpp magic.h
	g++ -g -std=c++14 -c magic.cpp -o magic.o

clean:
	rm -f *.o server client