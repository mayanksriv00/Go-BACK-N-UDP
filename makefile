#Makefile to run program
#./Server 2734 12 10
#./Client 127.0.0.1 2734 12 10

all: Server Client

Server: Server.cpp
	g++ Server.cpp -o Server

Client: Client.cpp
	g++ Client.cpp -o Client

clean:
	rm -f *.o

