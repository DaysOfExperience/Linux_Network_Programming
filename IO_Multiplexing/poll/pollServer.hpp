#pragma once

#include "Sock.hpp"
#include "Log.hpp"
#include <sys/poll.h>

#define FD_NONE -1

// 疑问：poll传数组，“传统方案”是数组中的空位的fd设置为-1，那我能不能直接设置为nullptr呢？哈哈哈...
// 怎么可能啊，这是一个结构体数组，你他妈怎么把结构体设置为nullptr，弱智一样
class PollServer
{
    const static int nfds = 100;

public:
    PollServer(const uint16_t &port = 8080) : _port(port), _nfds(nfds)
    {
        _listen_sock = Sock::Socket();
        Sock::Bind(_listen_sock, port);
        Sock::Listen(_listen_sock);

        _fds = new struct pollfd[_nfds];
        for (int i = 0; i < nfds; ++i)
        {
            _fds[i].fd = FD_NONE;
            _fds[i].events = 0;
        }
        _fds[0].fd = _listen_sock;
        _fds[0].events = POLLIN;
        _timeout = 3000;
    }
    void Start()
    {
        while (true)
        {
            PrintForDebug();
            int n = poll(_fds, _nfds, _timeout); // 1000ms,1s,阻塞等待1s // 3000
            if (n > 0)
            {
                std::cout << "event ready..." << std::endl;
                HandlerEvent();
            }
            else if (n == 0)
            {
                std::cout << "time out..." << std::endl;
                continue;
            }
            else
            {
                std::cout << "poll error..." << std::endl;
                continue;
            }
        }
    }
    ~PollServer()
    {
        if(_listen_sock >= 0)
            close(_listen_sock);
        if(_fds)
            delete [] _fds;
    }
private:
    void HandlerEvent()
    {
        // 有连接的读事件就绪
        for (int i = 0; i < _nfds; ++i)
        {
            if (_fds[i].fd == FD_NONE)
                continue;
            if (_fds[i].revents & POLLIN) // 该文件描述符读事件被关心且读事件就绪~
            {
                if (_fds[i].fd == _listen_sock)
                    Accepter();
                else
                    Recver(i);
            }
        }
    }
    void Accepter()
    {
        int sock = Sock::Accept(_listen_sock, nullptr, nullptr);
        if (sock < 0)
        {
            logMessage(WARNING, "accept error");
            return;
        }
        // 连接已经获取
        int pos;
        for (pos = 0; pos < _nfds; ++pos)
        {
            if (_fds[pos].fd == FD_NONE)
                break;
        }
        if (pos == _nfds)
        {
            logMessage(WARNING, "poll full, close connection: %d", sock);
            close(sock);
            return;
        }
        _fds[pos].fd = sock;
        _fds[pos].events = POLLIN;
    }
    void Recver(int i)
    {
        char buff[1024];
        ssize_t sz = recv(_fds[i].fd, buff, sizeof buff - 1, 0);
        if (sz > 0)
        {
            buff[sz] = 0;
            logMessage(NORMAL, "client[%d]# %s", _fds[i].fd, buff);
        }
        else if (sz == 0)
        {
            // client关闭连接
            logMessage(NORMAL, "client[%d] quit", _fds[i].fd);
            close(_fds[i].fd);
            _fds[i].fd = FD_NONE;
            _fds[i].events = 0;
        }
        else
        {
            // 读取失败
            logMessage(WARNING, "client[%d] recv error, close connection...", _fds[i].fd);
            close(_fds[i].fd);
            _fds[i].fd = FD_NONE;
            _fds[i].events = 0;
        }
    }
    void PrintForDebug()
    {
        std::cout << "_fds[]: ";
        for(int i = 0; i < _nfds; ++i)
        {
            if(_fds[i].fd == FD_NONE) continue;
            std::cout << _fds[i].fd << " ";
        }
        std::cout << std::endl;
    }
private:
    int _listen_sock;
    uint16_t _port;

    struct pollfd *_fds;   // 存储你所让poll关心的哪些文件描述符的哪些事件，以及poll返回之后哪些文件描述符的哪些事件就绪了
    nfds_t _nfds;

    int _timeout;
};