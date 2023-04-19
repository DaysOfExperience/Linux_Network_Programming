#ifndef _TCP_SERVER_HPP_
#define _TCP_SERVER_HPP_

#include "Sock.hpp"
#include <vector>
#include <functional>

// 说实话，这个TcpServer类实现的非常棒，真的很棒，网络和服务进行了解耦。
// 使用者直接BindServer, 然后start即可
namespace ns_tcpserver
{
    using func_t = std::function<void(int socket)>; // 服务器提供的服务方法类型void(int)，可变

    class TcpServer;
    class ThreadData
    {
    public:
        ThreadData(int sock, TcpServer *server)
            : _sock(sock), _server(server)
        {}
        ~ThreadData() {}
    public:
        int _sock;
        TcpServer *_server; // 因为静态成员函数呀
    };

    class TcpServer
    {
        // 不关心bind的ip和port，因为用不到啊，保留一个listen_sock用于accept就够了。
    private:
        int _listen_sock;
        Sock _sock;
        std::vector<func_t> _funcs; // 服务器提供的服务
    private:
        static void *threadRoutine(void *args)
        {
            pthread_detach(pthread_self());
            ThreadData *td = (ThreadData *)args;
            td->_server->excute(td->_sock); // 提供服务
            close(td->_sock);   // 保证四次挥手正常结束
            delete td;
            return nullptr;
        }
    public:
        TcpServer(const uint16_t &port, const std::string &ip = "0.0.0.0")
        {
            // 创建监听套接字，bind，listen
            _listen_sock = _sock.Socket();
            _sock.Bind(_listen_sock, port, ip);
            _sock.Listen(_listen_sock);
        }
        void start()
        {
            for (;;)
            {
                // 开始accept，然后执行任务
                std::string ip;
                uint16_t port;     // 这两个东西，也并没有传给线程。
                int sock = _sock.Accept(_listen_sock, &port, &ip); // 后面是输出型参数
                if (sock == -1)
                    continue; // 本次accept失败，循环再次accept。目前来看几乎不会

                // 连接客户端成功，ip port已有。但是这里没用...
                pthread_t tid;
                ThreadData *td = new ThreadData(sock, this);
                pthread_create(&tid, nullptr, threadRoutine, (void *)td); // 新线程去提供service，主线程继续accept
            }
        }
        void bindService(func_t service)   // 暴露出去的接口，用于设置该服务器的服务方法
        {
            _funcs.push_back(service);
        }
        void excute(int sock)
        {
            for (auto &func : _funcs)
            {
                func(sock);
            }
        }
        ~TcpServer()
        {
            if (_listen_sock >= 0)
                close(_listen_sock);
        }
    };
}
#endif