#include "log.hpp"
#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
static void Usage(char *proc)
{
    std::cout << "\nUsage : " << proc << " server_ip server_port\n"
              << std::endl;
}

void *in(void *args)
{
    // 接收server端发送的数据
    logMessage(NORMAL, "接收线程已启动");
    int sock = *(int *)args; // 套接字
    char message_svr[1024];  // 缓冲区
    while (true)
    {
        struct sockaddr_in server;
        socklen_t len = sizeof server;
        // 一直都在等待UDP报文，进行接收
        ssize_t sz = recvfrom(sock, message_svr, sizeof message_svr - 1, 0, (struct sockaddr *)&server, &len);
        if (sz > 0)
        {
            message_svr[sz] = 0;
            std::cout << message_svr << std::endl;
        }
    }
}

struct ThreadData
{
    ThreadData(int sock, const std::string &ser_ip, uint16_t ser_port)
        : _sock(sock), _server_ip(ser_ip), _server_port(ser_port)
    {
    }
    int _sock;
    std::string _server_ip;
    uint16_t _server_port;
};

void *out(void *args)
{
    // 发送client端发送的数据
    logMessage(NORMAL, "发送线程已启动");
    ThreadData *td = (ThreadData *)args;
    struct sockaddr_in server;
    memset(&server, 0, sizeof server);
    server.sin_port = htons(td->_server_port);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(td->_server_ip.c_str());
    std::string message;
    while (true)
    {
        // std::cout << "client# ";
        std::cerr << "client# ";  // 为了不让这个输出信息也输出到管道中！和服务端发来的进行区分！否则会出现乱码现象。
        // fflush(stdout);
        std::getline(std::cin, message);

        sendto(td->_sock, message.c_str(), message.size(), 0, (struct sockaddr *)&server, sizeof server);
    }
}

// ./udp_client 127.0.0.1 8080
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        Usage(argv[0]);
        exit(1);
    }
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        logMessage(FATAL, "socket : %d:%s", errno, strerror(errno));
        exit(2);
    }
    pthread_t itid, otid;
    pthread_create(&itid, nullptr, in, (void *)&sock);
    struct ThreadData td(sock, argv[1], atoi(argv[2]));
    pthread_create(&otid, nullptr, out, (void*)&td);

    pthread_join(itid, nullptr);
    pthread_join(otid, nullptr);
    return 0;
}