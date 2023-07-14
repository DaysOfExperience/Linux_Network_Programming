#include <memory>
#include "EpollServer.hpp"

void handler(int sock, std::string s)
{
    logMessage(NORMAL, "client[%d]# %s", sock, s.c_str());
}

int main()
{
    std::unique_ptr<EpollServer> es(new EpollServer(handler));
    es->Start();
    return 0;
}