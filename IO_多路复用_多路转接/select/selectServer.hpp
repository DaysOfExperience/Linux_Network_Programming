#pragma once
#include <iostream>
#include <sys/select.h>
#include "Sock.hpp"

#define FD_NONE -1
#define BITS 8
#define NUM sizeof(fd_set)*BITS

class SelectServer
{
    // const static int NUM = 1024;   // select 有一次处理文件描述符数量的最大上限（因为fd_set类型）

public:
    SelectServer(const uint16_t &port = 8080) : _port(port)
    {
        // Server:创建套接字，绑定，监听
        _listen_sock = Sock::Socket();
        Sock::Bind(_listen_sock, _port);
        Sock::Listen(_listen_sock);
        for (int i = 0; i < NUM; ++i)
        {
            _sock_array[i] = FD_NONE; // 无效文件描述符默认为-1，若非-1，则该文件描述符的读事件我们要让select关心
        }
        _sock_array[0] = _listen_sock;
    }
    void Start()
    {
        while (true)
        {
            DebugPrint();
            fd_set fs;
            FD_ZERO(&fs); // 对文件描述符集进行清空
            int max_fd = -1;
            for (int i = 0; i < NUM; ++i)
            {
                if (_sock_array[i] != FD_NONE)
                {
                    if (_sock_array[i] > max_fd)
                        max_fd = _sock_array[i];
                    FD_SET(_sock_array[i], &fs); // 对文件描述符进行关心
                }
            }
            // struct timeval;
            int ret = select(max_fd + 1, &fs, nullptr, nullptr, nullptr); // 阻塞式select多路转接
            if (ret > 0)
            {
                // 有文件描述符的读事件就绪
                logMessage(DEBUG, "get new events");
                HandlerEvents(fs);
            }
            else if (ret == 0)
            {
                logMessage(DEBUG, "time out...");
                continue;
            }
            else
            {
                logMessage(WARNING, "select error: %d : %s", errno, strerror(errno));
                // break;
                continue;
            }
        }
    }
    ~SelectServer()
    {
        if(_listen_sock >= 0)
            close(_listen_sock);
    }
private:
    void HandlerEvents(const fd_set &fs)
    {
        // DebugPrint();
        // fs中存储着当前读事件就绪的文件描述符
        for (int i = 0; i < NUM; ++i)
        {
            if (_sock_array[i] == FD_NONE)
                continue;
            if (FD_ISSET(_sock_array[i], &fs))
            {
                // 该文件描述符读事件就绪了，可以读了
                if (_sock_array[i] == _listen_sock)
                    Accepter();
                else
                    Recver(i); // 某文件描述符的读事件就绪了
            }
        }
    }
    void Accepter()
    {
        // logMessage(DEBUG, "for debug...");
        // listen套接字就绪，进行TCP连接的获取
        std::string ip;
        uint16_t port;
        int sock = Sock::Accept(_listen_sock, &port, &ip);  // Accpet有bug
        if (sock < 0)
        {
            logMessage(WARNING, "accept error");
            return;
        }
        // 接收到一个TCP连接
        // 此时应该将此文件描述符的事件关心交给select，而不是创建新线程或新进程或主进程去read，因此这里是多路复用服务器
        int i = 0;
        for (i = 0; i < NUM; ++i)
        {
            if (_sock_array[i] == FD_NONE)
                break;
        }
        if (i == NUM)
        {
            // select监管的文件描述符数量达到上限
            logMessage(WARNING, "select socket already full, sock:%d", sock);
            close(sock);
            return;
        }
        _sock_array[i] = sock; // 添加到这里之后，服务器下次select循环时，会自动将此文件描述符的读事件加入select关心中
    }
    void Recver(int pos)
    {
        logMessage(DEBUG, "sock:%d read event is ok...", _sock_array[pos]);
        char buff[1024];
        ssize_t sz = recv(_sock_array[pos], buff, sizeof buff - 1, 0);
        if (sz > 0)
        {
            buff[sz] = 0;
            logMessage(DEBUG, "client[%d]# %s", _sock_array[pos], buff);
        }
        else if (sz == 0)
        {
            // 此时对端关闭关闭连接
            logMessage(DEBUG, "client[%d] quit", _sock_array[pos]);
            close(_sock_array[pos]);
            _sock_array[pos] = FD_NONE;
        }
        else
        {
            // 读取错误
            logMessage(WARNING, "sock:%d read error:%d : %s", _sock_array[pos], errno, strerror(errno));
            close(_sock_array[pos]);
            _sock_array[pos] = FD_NONE;
        }
    }
    void DebugPrint()
    {
        std::cout << "_sock_array: ";
        for(int i = 0; i < NUM; ++i)
        {
            if(_sock_array[i] != FD_NONE)
            {
                std::cout << _sock_array[i] << " ";
            }
        }
        std::cout << std::endl;
    }
private:
    int _listen_sock; // listen文件描述符
    uint16_t _port;
    int _sock_array[NUM]; // select server要对哪些文件描述符进行读事件关心
}; 