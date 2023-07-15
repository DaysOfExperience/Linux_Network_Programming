#ifndef __TCP_SERVER_HPP__
#define __TCP_SERVER_HPP__

#include "Epoll.hpp"
#include "Sock.hpp"
#include "Log.hpp"
#include "Protocol.hpp"
#include <unordered_map>
#include <functional>
#include <vector>


class Connection;
class TcpServer;

using func_t = std::function<void (Connection*)>;
using hanlder_t = std::function<void (Connection*, std::string& request)>;
using namespace ns_protocol;

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
        AddConnection(_listensock, std::bind(&TcpServer::Accepter, this, std::placeholders::_1), nullptr, nullptr);

        // 4. 设定_revs，也就是存储就绪的事件
        _revs = new struct epoll_event[_revs_num];
        logMessage(DEBUG, "TcpServer init success~");
    }
    void AddConnection(int sock, func_t recv_cb = nullptr, func_t send_cb = nullptr, func_t except_cb = nullptr)
    {
        // 添加一个连接到TcpServer中
        // a. 添加到connections内
        // b. 添加到epoll中
        Connection *conn = new Connection(sock);
        conn->SetCallBack(recv_cb, send_cb, except_cb);
        conn->_server_ptr = this;

        // 添加到epoll中
        _epoll.AddEvent(sock, EPOLLIN | EPOLLET);  // 每一个事件添加进epoll时，默认都是关心读，还有ET模式
        // 设置非阻塞！因为这个epoll是ET模式
        Sock::SetNonBlock(sock);
        // 添加到connections内
        _connections.insert(std::make_pair(sock, conn));
    }
    void Dispather(hanlder_t handler)
    {
        _handler = handler;  // 对每一个client传来的应用层报文进行业务处理
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
        // 有事件就绪，在_revs中
        for(int i = 0; i < n; ++i)
        {
            // 什么套接字的什么事件就绪了
            int sock = _revs[i].data.fd;
            int revents = _revs[i].events;
            bool exists = ConnIsExists(sock);
            Connection *conn = nullptr;
            if(exists)
                conn = _connections[sock];
            if(revents & EPOLLERR || revents & EPOLLHUP)
            {
                // 异常事件就绪
                if(ConnIsExists(conn->_sock) && conn->_except_cb != nullptr)
                    conn->_except_cb(conn);
            }
            if(revents & EPOLLIN)
            {
                // 读事件就绪，不需要判断是listen还是普通
                if(ConnIsExists(conn->_sock) && conn->_recv_cb != nullptr)
                    conn->_recv_cb(conn);
            }
            if(revents & EPOLLOUT)
            {
                if(ConnIsExists(conn->_sock) && conn->_send_cb != nullptr)
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
    // 下面是所有连接的读，写，异常方法(listen套接字&常规套接字)
    void Accepter(Connection *conn)
    {
        // listen套接字的读方法
        // ET模式，需要循环读取完此轮所有的TCP连接
        while(true)
        {
            int accept_errno;
            std::string client_ip;
            uint16_t client_port;
            int sock = Sock::Accept(_listensock, &client_port, &client_ip, &accept_errno);
            if(sock < 0)
            {
                // accept失败 1. 真的读取失败 2. 因为没有连接了
                // 我们需要注意的是没有链接时(主要就是这里，没其他的)
                if(accept_errno == EAGAIN || accept_errno == EWOULDBLOCK)
                {
                    // 没有建立好的TCP连接可以获取了，本轮accept结束
                    break;
                }
                else if(accept_errno == EINTR)
                {
                    continue;  // 概率非常低
                }
                else
                {
                    logMessage(WARNING, "accept error, %d : %s", accept_errno, strerror(accept_errno));
                    conn->_except_cb(conn);
                    break;
                }
            }
            AddConnection(sock
            , std::bind(&TcpServer::Recver, this, std::placeholders::_1)
            , std::bind(&TcpServer::Sender, this, std::placeholders::_1)
            , std::bind(&TcpServer::Excepter, this, std::placeholders::_1));
            logMessage(DEBUG, "get a new TCP link: ip:%s, port:%d, sock:%d", client_ip.c_str(), client_port, sock);
        }
    }
    void EnableRW(Connection *conn, bool read, bool write)
    {
        uint32_t events = (read ? EPOLLIN : 0 ) | (write ? EPOLLOUT : 0);
        _epoll.ModEvent(conn->_sock, events);
    }
    // 常规套接字的读 写 异常方法
    // 读：读取到client传来的序列化后的+报头的应用层报文，读入到接收缓冲区中
    void Recver(Connection *conn)
    {
        // ET模式，循环读取，最后会以出错的方式告诉你读完数据了，没有数据了
        char buff[10240];
        bool handle = true;
        while(true)
        {
            ssize_t sz = recv(conn->_sock, buff, sizeof buff - 1, 0);
            if(sz < 0)
            {
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                else if(errno == EINTR)
                {
                    continue;
                }
                else
                {
                    // 读取失败
                    logMessage(WARNING, "recv error %d : %s", conn->_sock, errno, strerror(errno));
                    handle = false;
                    conn->_except_cb(conn);
                    break;
                }
            }
            else if(sz == 0)
            {
                // client关闭连接
                logMessage(DEBUG, "client[%d] quit, server close too...", conn->_sock);
                handle = false;
                conn->_except_cb(conn);
                break;
            }
            else
            {
                buff[sz] = 0;
                conn->_in_buffer += buff; // TCP面向字节流
            }
        }
        // 至此可能是本轮数据读取完毕则处理，可能是读取出错，可能是对端关闭连接(不处理)
        if(!handle) return;
        // 处理接收缓冲区中的数据：缓冲区中包括多少个应用层报文是不确定的
        std::vector<std::string> messages;
        SplitMessage(conn->_in_buffer, messages);
        for(auto &s : messages) 
        {
            _handler(conn, s);  // 某一个连接的应用层报文
        }
    }
    void Sender(Connection *conn)
    {
        // 一般来说，一旦server要发送的数据写入到应用层的发送缓冲区，且使写事件被epoll关心
        // 而此时TCP的发送缓冲区初始时都是有空间的，epoll就会直接检测到写事件就绪
        // 我们需要循环发送，直到把应用层的发送缓冲区中数据发完
        while(true)
        {
            ssize_t n = send(conn->_sock, conn->_out_buffer.c_str(), conn->_out_buffer.size(), 0);
            if(n > 0)
            {
                conn->_out_buffer.erase(0, n);
                if(conn->_out_buffer.empty()) break;  // 本轮发送完毕~
                // 如果确实是发完了，下次你不break，相当于send空的，这和严格意义上的EAGAIN不服
            }
            else
            {
                // 可能TCP的发送缓冲区（不是_out_buffer）满了，发送条件不就绪
                // 如同接收缓冲区空了，也就是本轮任务结束
                if(errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                else if(errno == EINTR)
                    continue;
                else
                {
                    // send失败
                    logMessage(WARNING, "send error, %d : %s", errno, strerror(errno));
                    conn->_except_cb(conn);
                    break;
                }
            }
        }
        // 1. TCP发送缓冲区没满，但是已经把该发的都发了
        // 2. TCP发送缓冲区满了，发送条件不就绪，此时就是相当于IO = 等 + 拷贝，该等了
        // 3. 发送出错了~
        if(conn->_out_buffer.empty())
            EnableRW(conn, true, false);
    }
    void Excepter(Connection *conn)
    {
        if(!ConnIsExists(conn->_sock)) return;
        // 存在
        _connections.erase(conn->_sock);
        _epoll.DelEvent(conn->_sock);
        close(conn->_sock);
        delete conn;
    }
private:
    int _listensock;
    uint16_t _port;
    Epoll _epoll;

    std::unordered_map<int, Connection*> _connections;

    struct epoll_event *_revs;
    int _revs_num;

    hanlder_t _handler;   // std::function<void (Connection*, string& request)>;
    // 对于每一个应用层报文（序列化+报头），通过这个方法进行业务处理
};


#endif