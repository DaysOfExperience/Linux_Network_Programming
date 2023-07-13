#ifndef __TCP_SERVER_HPP__
#define __TCP_SERVER_HPP__

#include "Epoll.hpp"
#include "Sock.hpp"
#include "Log.hpp"
#include <unordered_map>
#include <functional>

class Connection;
using func_t = std::function<void (Connection*)>;

class Connection
{
public:
    Connection(int sock = -1)
    :_sock(sock), _server_ptr(nullptr)
    {}
    void SetCallBack(func_t recv_cb = nullptr, func_t send_cb = nullptr, func_t except_cb = nullptr)
    {
        _recv_cb = recv_cb;
        _send_cb = send_cb;
        _except_cb = except_cb;
    }
    ~Connection()
    {}
public:
    int _sock;

    func_t _recv_cb;   // _sock文件描述符对应的读方法
    func_t _send_cb;   // _sock文件描述符对应的写方法
    func_t _except_cb; // _sock文件描述符对应的异常方法

    std::string _in_buffer;   // 输入缓冲区
    std::string _out_buffer;  // 输出缓冲区

    TcpServer *_server_ptr;
};

class TcpServer
{
    const static uint16_t gport = 8080;
    const static int gnum = 100;
    const static int gtimeout = 5000;
public:
    TcpServer(const uint16_t &port = 8080, int timeout = gtimeout)
    :_port(port), _revs_num(gnum), _epoll(timeout)
    {
        // 1. 创建listen套接字，绑定，监听
        _listensock = Sock::Socket();
        Sock::Bind(_listensock, _port);
        Sock::Listen(_listensock);

        // 2. 创建epoll模型，理解为_epoll就是一个epoll模型
        _epoll.Create();

        // 3. 将listen套接字添加到epoll中
        AddConnection(_listensock, std::bind(Accepter, this, std::placeholders::_1), nullptr, nullptr);
        // 4. 设定_revs，也就是存储就绪的事件
        _revs = new struct epoll_event[_revs_num];
    }
    void AddConnection(int sock, func_t recv_cb = nullptr, func_t send_cb = nullptr, func_t except_cb = nullptr)
    {
        // 设置非阻塞！因为这个epoll是ET模式

        // 添加一个连接到TcpServer中
        // a. 添加到connections内
        // b. 添加到epoll中
        Connection *conn = new Connection(sock);
        conn->SetCallBack(recv_cb, send_cb, except_cb);
        conn->_server_ptr = this;

        // 添加到epoll中
        _epoll.AddEvent(sock, EPOLLIN | EPOLLET);  // 每一个事件添加进epoll时，默认都是关心读，还有ET模式
        // 添加到connections内
        _connections.insert(std::make_pair(sock, conn));
    }
    void Accepter(Connection *conn)
    {

    }
    void Recver(Connection *conn)
    {

    }
    void Sender(Connection *conn)
    {

    }
    void Excepter(Connection *conn)
    {

    }
    void Dispather()
    {
        while(true)
        {
            LoopOnce();
        }
    }
    void LoopOnce()
    {
        int n = _epoll.Wait(_revs, _revs_num);
        if(n > 0)
        {
            // 有事件就绪
            logMessage(DEBUG, "events ready");
            HandlerEvents(n);
        }
        else if(n == 0)
        {
            logMessage(DEBUG, "epoll wait timeout...");
        }
        else
        {
            logMessage(WARNING, "epoll wait error, errno:%d, strerror:%s", errno, strerror(errno));
        }
    }
    void HandlerEvents(int n)
    {
        // 有事件就绪
        for(int i = 0; i < n; ++i)
        {
            // 什么套接字的什么事件就绪了
            int sock = _revs[i].data.fd;
            int revents = _revs[i].events;
            bool exists = ConnIsExists(sock);
            Connection *conn = nullptr;
            if(exists)
                conn = _connections[sock];
            // if(revents & EPOLLERR || revents & EPOLLHUP)
            // {
            //     // 异常事件就绪
            //     _connections[sock]->_except_cb(_connections[sock]);
            // }
            if(revents & EPOLLIN)
            {
                // 读事件就绪，不需要判断是listen还是普通
                if(exists && conn->_recv_cb != nullptr)
                    conn->_recv_cb(conn);
            }
            if(revents & EPOLLOUT)
            {
                if(exists && conn->_send_cb != nullptr)
                    conn->_send_cb(conn);
            }
        }
    }
    bool ConnIsExists(int sock)
    {
        return _connections.find(sock) != _connections.end();
    }
    ~TcpServer()
    {
        if(_listensock >= 0) close(_listensock);
        if(_revs) delete[] _revs;
    }
private:
    int _listensock;
    uint16_t _port;
    Epoll _epoll;

    std::unordered_map<int, Connection*> _connections;

    struct epoll_event *_revs;
    int _revs_num;
};


#endif