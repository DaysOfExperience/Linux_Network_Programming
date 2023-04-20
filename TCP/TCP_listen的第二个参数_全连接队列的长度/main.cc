#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "Sock.hpp"

int main()
{
    Sock sock;
    int listen_sock = sock.Socket();
    sock.Bind(listen_sock, 8080);
    sock.Listen(listen_sock, 3);

    while (true)
    {
        sleep(5);
        std::string ip;
        uint16_t port;
        sock.Accept(listen_sock, &port, &ip);
    }
    return 0;
}