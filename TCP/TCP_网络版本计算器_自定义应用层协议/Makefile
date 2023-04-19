.PHNOY:all
all: server client

server:CalServer.cc
	g++ -o $@ $^ -std=c++11 -lpthread -ljsoncpp
client:CalClient.cc
	g++ -o $@ $^ -std=c++11 -ljsoncpp

.PHONY:remake
remake:
	rm -f server client; make 

.PHONY:clean
clean:
	rm -f server client