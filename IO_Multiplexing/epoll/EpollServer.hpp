#ifndef __EPOLL_SERVER_HPP__
#define __EPOLL_SERVER_HPP__

#include <iostream>
#include <functional>
#include <string>
#include "Log.hpp"
#include "Sock.hpp"
#include "Epoll.hpp"


// 这里的epoll服务器只处理读取
class EpollServer
{
    using func_t = std::function<void(int sock, std::string)>;
public:
    EpollServer(func_t HandlerRequest, const uint16_t& port = 8080, int revs_num = 100)
    : _handler_request(HandlerRequest), _port(port), _revs_num(revs_num)
    {
        // 0. 注意，这里的epollserver的接收就绪事件的数组，是需要我们开辟的
        // 不然你直接传一个指针，并且没有初始化过的野指针给OS，是什么意思？
        _revs = new struct epoll_event[_revs_num];

        _listen_sock = Sock::Socket();
        Sock::Bind(_listen_sock, _port);
        Sock::Listen(_listen_sock);

        _epfd = Epoll::EpollCreate();
        if(!Epoll::EpollCtl(_epfd, EPOLL_CTL_ADD, _listen_sock, EPOLLIN))
            exit(6); // EPOLLIN EPOLLOUT EPOLLERR
        logMessage(DEBUG, "epoll server init success, epfd:%d", _epfd);
    }
    void Start()
    {
        int timeout = -1;
        while(true)
        {
            LoopOnce(timeout);
        }
    }
    void LoopOnce(int timeout)
    {
        // 进行一次epoll_wait
        int n = Epoll::EpollWait(_epfd, _revs, _revs_num, timeout);  // -1阻塞, 0非阻塞
        if(n > 0)
        {
            logMessage(DEBUG, "events ready");
            HandlerEvents(n);  // 就绪文件描述符的事件在_revs中
        }
        else if(n == 0)
        {
            logMessage(DEBUG, "epoll wait timeout..."); // 正常情况，这个timeout不需要输出出来hhhh
        }
        else
        {
            logMessage(WARNING, "epoll wait error, errno:%d, strerror:%s", errno, strerror);
        }
    }
    void HandlerEvents(int n)
    {
        // n 其实就是_revs这个指针指向数组的元素的个数
        // 也就是就绪事件的个数
        for(int i = 0; i < n; ++i)
        {
            uint32_t revents = _revs[i].events;
            int sock = _revs[i].data.fd;
            if(revents & EPOLLIN)
            {
                if(sock == _listen_sock) Accepter();
                else Recver(sock);
            }
            if(revents & EPOLLOUT)
            {
                // 这样不对其实，但是，这个epollserver只是为了简单学习一下
            }
            if(revents & EPOLLERR)
            {
                // ...
            }
        }
    }
    void Accepter()
    {
        int sock = Sock::Accept(_listen_sock, nullptr, nullptr);
        if(sock < 0)
        {
            logMessage(WARNING, "accept error");
            return;
        }
        Epoll::EpollCtl(_epfd, EPOLL_CTL_ADD, sock, EPOLLIN);
        logMessage(DEBUG, "accept success, new sock:%d", sock);
    }
    void Recver(int sock)
    {
        char buff[1024];
        ssize_t sz = recv(sock, buff, sizeof buff - 1, 0);
        if(sz > 0)
        {
            buff[sz] = 0;
            _handler_request(sock, buff);
        }
        else if(sz == 0)
        {
            // 1. 先在epoll中删除对文件描述符的关心
            Epoll::EpollCtl(_epfd, EPOLL_CTL_DEL, sock, 0);
            // 2. 关闭文件描述符
            close(sock);
            logMessage(NORMAL, "client quit, sock:%d", sock);
        }
        else
        {
            // 1. 先在epoll中删除对文件描述符的关心
            Epoll::EpollCtl(_epfd, EPOLL_CTL_DEL, sock, 0);
            // 2. 关闭文件描述符
            close(sock);
            logMessage(WARNING, "recv error, sock:%d", sock);
        }
    }
    ~EpollServer()
    {
        if(_listen_sock >= 0) close(_listen_sock);
        if(_epfd >= 0) close(_epfd);
        if(_revs) delete[] _revs;
    }
private:
    uint16_t _port;
    int _listen_sock;

    int _epfd;
    struct epoll_event *_revs;   // 通过epoll_wait获取就绪的文件描述符的就绪事件
    int _revs_num;

    func_t _handler_request;
};


#endif