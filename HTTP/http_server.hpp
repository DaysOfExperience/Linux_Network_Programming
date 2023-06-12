#pragma once
#include "Sock.hpp"
#include <functional>
#include <signal.h>

// #include <string>
using func_t = std::function<void(int)>;

class HttpServer
{
public:
    HttpServer(uint16_t port, func_t func)
    : func_(func)
    {
        listensock_ = sock_.Socket();
        sock_.Bind(listensock_, port);
        sock_.Listen(listensock_);
    }
    void start()
    {
        signal(SIGCHLD, SIG_IGN);
        // 开始accept
        while(true)
        {
            uint16_t client_port;
            std::string client_ip;
            int sock = sock_.Accept(listensock_, &client_port, &client_ip); // 获取到一个client连接
            if(fork() == 0)
            {
                // 子进程
                close(listensock_);
                func_(sock);
                close(sock);
                exit(0);
            }
            close(sock);
        }
    }
private:
    int listensock_;
    Sock sock_;
    func_t func_;
};