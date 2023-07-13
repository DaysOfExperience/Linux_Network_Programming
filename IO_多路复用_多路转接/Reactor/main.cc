#include "TcpServer.hpp"
#include <memory>

int main()
{
    std::unique_ptr<TcpServer> ts(new TcpServer());
    ts->Dispather();
    return 0;
}