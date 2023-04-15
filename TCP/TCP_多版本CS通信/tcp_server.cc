#include "tcp_server.hpp"
#include <iostream>
#include <memory>

static void Usage(char *proc)
{
    std::cout << "\nUsage: " << proc << " port\n" << std::endl;
}

// ./tcp_server 8080
int main(int argc, char **argv)
{
    if(argc != 2)
    {
        Usage(argv[0]);
        exit(1);
    }
    std::unique_ptr<TcpServer> svr(new TcpServer(atoi(argv[1])));
    svr->initServer();
    svr->start();
    return 0;
}