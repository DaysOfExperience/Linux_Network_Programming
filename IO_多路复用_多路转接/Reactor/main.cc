#include "TcpServer.hpp"
#include <memory>

int main()
{
    std::unique_ptr<TcpServer> ts(new TcpServer());
    return 0;
}