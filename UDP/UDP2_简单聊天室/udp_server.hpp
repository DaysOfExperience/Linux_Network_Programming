#ifndef _UDP_SERVER_HPP_
#define _UDP_SERVER_HPP_

#include "log.hpp"
#include <string>
#include <cstring>
#include <unordered_map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// 基于UDP协议的服务端
class UdpServer
{
public:
    UdpServer(uint16_t port, std::string ip = "")
        : _port(port), _ip(ip), _sock(-1)
    {
    }
    void initServer()
    {
        // 1.创建套接字
        _sock = socket(AF_INET, SOCK_DGRAM, 0); // 1.套接字类型：网络套接字(不同主机间通信) 2.面向数据报还是面向字节流：UDP面向数据报
        // SOCK_DGRAM支持数据报（固定最大长度的无连接、不可靠消息）。
        if (_sock < 0)
        {
            // 创建套接字失败?
            logMessage(FATAL, "%d:%s", errno, strerror(errno));
            exit(2);
        }
        // 2.bind : 将用户设置的ip和port在内核中和我们当前的进程强关联
        struct sockaddr_in local; // 传给bind的第二个参数，存储ip和port的信息。
        local.sin_family = AF_INET;
        // 服务器的IP和端口未来也是要发送给对方主机的 -> 先要将数据发送到网络！-> 故需要转换为网络字节序
        local.sin_port = htons(_port); // host->network l 16
        // "192.168.110.132" -> 点分十进制字符串风格的IP地址,每一个区域取值范围是[0-255]: 1字节 -> 4个区域，4字节
        // INADDR_ANY：让服务器在工作过程中，可以从本机的任意IP中获取数据（一个服务器可能不止一个ip，(这块有些模糊）
        local.sin_addr.s_addr = _ip.empty() ? INADDR_ANY : inet_addr(_ip.c_str());
        // 点分十进制字符串风格的IP地址 <-> 4字节整数   4字节主机序列 <-> 网络序列   inet_addr可完成上述工作
        if (bind(_sock, (struct sockaddr *)&local, sizeof local) < 0) // ！！！
        {
            logMessage(FATAL, "bind : %d:%s", errno, strerror(errno));
            exit(2);
        }
        logMessage(NORMAL, "init udp server %s...", strerror(errno));
    }
    void start()
    {
        // 作为一款网络服务器，永远不退出的！-> 服务器启动-> 进程 -> 常驻进程 -> 永远在内存中存在，除非挂了！
        char buff_message[1024]; // 存储从client发来的数据
        for (;;)
        {
            struct sockaddr_in peer;     // 输出型参数
            socklen_t len = sizeof peer; // 输出+输入型参数
            memset(&peer, 0, len);

            // 读取client发来的数据
            ssize_t sz = recvfrom(_sock, buff_message, sizeof(buff_message) - 1, 0, (struct sockaddr *)&peer, &len); // 哪个ip/port给你发的
            // receive a message from a socket，从一个套接字（或许对应网卡）中接收信息
            buff_message[sz] = 0;
            uint16_t cli_port = ntohs(peer.sin_port);      // 从网络中来，转换为主机序列！哈哈哈
            std::string cli_ip = inet_ntoa(peer.sin_addr); // 4字节网络序列ip->字符串风格的IP
            char key[128];
            snprintf(key, sizeof key, "%s-%d", cli_ip.c_str(), cli_port);
            logMessage(NORMAL, "[%s:%d] clinet worked", cli_ip.c_str(), cli_port);
            // printf("[%s:%d]# %s\n", cli_ip.c_str(), cli_port, buff_message);    // 可有可无...服务端显示一下客户端发来了什么
            if (_map.find(key) == _map.end())
            {
                _map.insert({key, peer});
                logMessage(NORMAL, "[%s:%d] client joined", cli_ip.c_str(), cli_port);
            }
            // if (sz > 0)
            // {
            //     // 从client获取到了非空数据，client端的ip/port信息存储在peer中。
            //     buff_message[sz] = 0;
            //     uint16_t cli_port = ntohs(peer.sin_port); // 从网络中来，转换为主机序列！哈哈哈
            //     std::string cli_ip = inet_ntoa(peer.sin_addr); // 4字节网络序列ip->字符串风格的IP
            //     snprintf(key, sizeof key, "%s-%d", cli_ip.c_str(), cli_port);
            //     logMessage(NORMAL, "[%s:%d] clinet worked", cli_ip.c_str(), cli_port);
            //     // printf("[%s:%d]# %s\n", cli_ip.c_str(), cli_port, buff_message);    // 可有可无...服务端显示一下客户端发来了什么
            //     if(_map.find(key) == _map.end())
            //     {
            //         _map.insert({key, peer});
            //         logMessage(NORMAL, "[%s:%d] client joined", cli_ip.c_str(), cli_port);
            //     }
            // }
            // else
            // {
            //     // 这里的逻辑是：如果client端最初发送空数据，则不加入群聊。
            //     buff_message[0] = 0;
            // }
            // 群聊服务端，此时需要给所有群聊中的client，发送某client发来的数据
            for (auto &iter : _map)
            {
                // if(iter.second.sin_port != peer.sin_port)
                std::string sendMessage(key);
                sendMessage += "# ";
                sendMessage += buff_message;
                sendto(_sock, sendMessage.c_str(), sendMessage.size(), 0, (struct sockaddr *)&iter.second, sizeof iter.second);
            }
        }
    }
    ~UdpServer()
    {
        close(_sock);
    }
private:
    // 一个服务器，一般必须需要ip地址和port(16位的整数)
    uint16_t _port;  // 端口号
    std::string _ip; // ip
    int _sock;
    std::unordered_map<std::string, struct sockaddr_in> _map; // [ip:port] | struct
};

#endif