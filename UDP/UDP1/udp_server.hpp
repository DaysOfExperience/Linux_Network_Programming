#ifndef _UDP_SERVER_HPP_
#define _UDP_SERVER_HPP_

#include "log.hpp"
#include <string>
#include <cstring>
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
        // 作为一款网络服务器，永远不退出的！-> 服务器启动 -> 进程 -> 常驻进程 -> 永远在内存中存在，除非挂了！
        char buff_message[1024]; // 存储从client发来的数据
        char back_message[124];  // 辅助popen:模拟命令行执行
        std::string back_str;    // 返回给客户端的数据
        for (;;)
        {
            struct sockaddr_in peer; // 输出型参数
            memset(&peer, 0, sizeof peer);
            socklen_t len = sizeof peer; // 输出+输入型参数

            // 读取client发来的数据
            ssize_t sz = recvfrom(_sock, buff_message, sizeof(buff_message) - 1, 0, (struct sockaddr *)&peer, &len);  // 哪个ip/port给你发的
            // receive a message from a socket，从一个套接字（或许对应网卡）中接收信息
            if (sz > 0)
            {
                // 从client获取到了数据
                buff_message[sz] = 0;
                // 你发过来的字符串是指令 ls -a -l, rm -rf ~
                if(strcasestr(buff_message, "rm") != nullptr)
                {
                    std::string err_msg = "可恶 : ";
                    std::cout << inet_ntoa(peer.sin_addr)<< " : "<< ntohs(peer.sin_port)  << " # "<< err_msg << buff_message << std::endl;
                    sendto(_sock, err_msg.c_str(), err_msg.size(), 0, (struct sockaddr*)&peer, len);
                    continue;
                }
                FILE* fp = popen(buff_message, "r");
                if(fp == nullptr)
                {
                    logMessage(ERROR, "popen : %d:%s\n", errno, strerror(errno));
                    continue;
                }
                while(fgets(back_message, sizeof(back_message), fp) != nullptr)
                {
                    back_str += back_message;
                }
                fclose(fp);
            }
            else
            {
                // back_str.clear();
            }
            // 作为一款伪shell server，任务就是写回client发来的命令的结果，结果存储在back_str中
            sendto(_sock, back_str.c_str(), back_str.size(), 0, (struct sockaddr *)&peer, len);
            back_str.clear();
        }
    }
    void start1()
    {
        // 作为一款网络服务器，永远不退出的！-> 服务器启动-> 进程 -> 常驻进程 -> 永远在内存中存在，除非挂了！
        char buff_message[1024]; // 存储从client发来的数据
        for (;;)
        {
            struct sockaddr_in peer; // 输出型参数
            memset(&peer, 0, sizeof peer);
            socklen_t len = sizeof peer; // 输出+输入型参数
            // 读取client发来的数据
            ssize_t sz = recvfrom(_sock, buff_message, sizeof(buff_message) - 1, 0, (struct sockaddr *)&peer, &len);  // 哪个ip/port给你发的
            // receive a message from a socket，从一个套接字（或许对应网卡）中接收信息
            if (sz > 0)
            {
                // 从client获取到了非空数据
                buff_message[sz] = 0;
                uint16_t cli_port = ntohs(peer.sin_port);
                std::string cli_ip = inet_ntoa(peer.sin_addr); // 4字节网络序列ip->字符串风格的IP
                printf("[%s:%d]# %s\n", cli_ip.c_str(), cli_port, buff_message);    // 可有可无...服务端显示一下客户端发来了什么
            }
            else
            {
                buff_message[0] = 0;
            }
            // 作为一款echo server，任务就是写回client发来的数据
            sleep(5);
            // sendto(_sock, buff_message, strlen(buff_message), 0, (struct sockaddr *)&peer, len);
            sendto(_sock, "", strlen(""), 0, (struct sockaddr *)&peer, len);
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
    int _sock;   // 套接字
};

#endif