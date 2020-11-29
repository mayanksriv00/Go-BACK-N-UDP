#Makefile to run program
#./Server 2734 12
#./Client 127.0.0.1 2734 12 10

all: Server Client

Server: Server_folder/Server.cpp
	g++ Server_folder/Server.cpp -o Server_folder/Server

Client: Client_folder/Client.cpp
	g++ Client_folder/Client.cpp -o Client_folder/Client

clean:
	rm -f Server_folder/*.o
	rm -f Client_folder/*.o
	rm -f Server_folder/Server
	rm -f Client_folder/Client

