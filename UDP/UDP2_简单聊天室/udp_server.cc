#include "udp_server.hpp"
#include <iostream>
#include <memory>
#include <cstdlib>

static void Usage(const char *proc)
{
    std::cout << "\nUsage: " << proc << " port\n"
              << std::endl;
}

// 格式：./udp_server 8080
// 疑问: 为什么不需要传ip?
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        Usage(argv[0]);
        exit(1);
    }
    uint16_t port = atoi(argv[1]);
    std::unique_ptr<UdpServer> svr(new UdpServer(port));
    svr->initServer();
    svr->start();
    return 0;
}