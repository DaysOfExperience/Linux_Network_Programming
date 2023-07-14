.PHONY:all
all: tcp_server tcp_client

tcp_server:Server.cc
    g++ -o $@ $^ -std=c++11

tcp_client:Client.cc
	g++ -o $@ $^ -std=c++11

.PHONY:clean
clean:
	rm -f tcp_server