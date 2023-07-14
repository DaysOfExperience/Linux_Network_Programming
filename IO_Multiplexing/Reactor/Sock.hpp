#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "Log.hpp"

// 对于一些TCP相关调用的封装
class Sock
{
private:
    const static int gback_log = 20;
public:
    static int Socket()
    {
        // 1. 创建套接字，成功返回对应套接字，失败直接进程exit
        int listen_sock = socket(AF_INET, SOCK_STREAM, 0); // 网络套接字, 面向字节流（tcp）
        if (listen_sock < 0)
        {
            logMessage(FATAL, "create listen socket error, %d:%s", errno, strerror(errno));
            exit(2);
        }
        // logMessage(NORMAL, "create listen socket success: %d", listen_sock); // 1111Log
        return listen_sock;
    }
    static void Bind(int sock, uint16_t port, std::string ip = "0.0.0.0")
    {
        // 2. bind，注意云服务器不能绑定公网IP，不允许。
        // 成功bind则成功bind，失败进程exit(bind不在循环语句内，故失败直接进程退出。)
        struct sockaddr_in local;
        memset(&local, 0, sizeof local);
        local.sin_family = AF_INET;
        local.sin_port = htons(port);
        local.sin_addr.s_addr = inet_addr(ip.c_str());
        if (bind(sock, (struct sockaddr *)&local, sizeof local) < 0)
        {
            logMessage(FATAL, "bind error, %d:%s", errno, strerror(errno));
            exit(3);
        }
    }
    static void Listen(int sock)
    {
        // 3. listen监听: 因为TCP是面向连接的，在我们正式通信之前，需要先建立连接
        // listen: 将套接字状态设置为监听状态。服务器要一直处于等待状态，这样客户端才能随时随地发起连接。
        // 成功则成功，失败则exit
        if (listen(sock, gback_log) < 0) // gback_log后面讲，全连接队列的长度。我猜测就是这个服务器同一时刻允许连接的客户端的数量最大值？也不太对呀，这么少么？
        {
            logMessage(FATAL, "listen error, %d:%s", errno, strerror(errno));
            exit(4);
        }
        // logMessage(NORMAL, "listen success");
    }
    // 一般经验
    // const std::string &: 输入型参数
    // std::string *: 输出型参数
    // std::string &: 输入输出型参数
    static int Accept(int sock, uint16_t *port, std::string *ip, int *accept_errno = nullptr)
    {
        // accept失败进程不退出，返回-1
        // 成功则返回对应的通信套接字
        struct sockaddr_in client;
        socklen_t len = sizeof client;
        // 其实accept是获取已经建立好的TCP连接。建立好的连接在一个内核队列中存放，最大数量的第二个参数+1
        int service_sock = accept(sock, (struct sockaddr *)&client, &len); // 返回一个用于与客户端进行网络IO的套接字，不同于listen_sock
        // On success, these system calls return a nonnegative integer that is a descriptor for the accepted socket.  On error, -1 is returned, and errno is set appropriately.
        if (service_sock < 0)
        {
            // logMessage(ERROR, "accept error, %d:%s", errno, strerror(errno));
            if(accept_errno)
                *accept_errno = errno;
            return -1;  // accept失败不直接exit，而是返回-1。因为在循环语句内部。
        }
        if (port)
            *port = ntohs(client.sin_port);
        if (ip)
            *ip = inet_ntoa(client.sin_addr);
        // if(ip && port)
        //     logMessage(NORMAL, "link(accept) success, service socket: %d | %s:%d", service_sock,
        //                (*ip).c_str(), *port);
        // else
        //     logMessage(NORMAL, "link(accept) success, service socket: %d", service_sock);
        return service_sock;
    }
    static int Connect(int sock, const std::string &ip, const uint16_t &port)
    {
        // 惯例写一下：失败返回-1，成功则客户端与服务端连接成功，返回0
        struct sockaddr_in server;
        memset(&server, 0, sizeof server);
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = inet_addr(ip.c_str());
        server.sin_port = htons(port);
        if (connect(sock, (struct sockaddr *)&server, sizeof server) < 0)
        {
            return -1;
        }
        return 0;
    }
    static void SetNonBlock(int sock)
    {
        int fl = fcntl(sock, F_GETFL);
        fcntl(sock, F_SETFL, fl | O_NONBLOCK);
    }
public:
    Sock() = default;
    ~Sock() = default;
};