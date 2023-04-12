#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static void Usage(char *proc)
{
    std::cout << "\nUsage : " << proc << " server_ip server_port\n"
              << std::endl;
}

// ./udp_client 127.0.0.1 8080
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        Usage(argv[0]);
        exit(1);
    }
    int sock = socket(AF_INET, SOCK_DGRAM, 0); // 网络套接字，udp面向数据报
    if(sock < 0)
    {
        std::cerr << "socket error" << std::endl;
        exit(2);
    }
    // client要不要bind？？要，但是一般client不会显式地bind，程序员不会自己bind
    // client是一个客户端 -> 普通人下载安装启动使用的-> 如果程序员自己bind了->
    // client 一定bind了一个固定的ip和port，万一，其他的客户端提前占用了这个port呢？？
    // 故client一般不需要显示的bind指定port，而是让OS自动随机选择(什么时候进行的呢？见下方)

    std::string message;     // 用户输入数据的缓冲区

    struct sockaddr_in server;      
    memset(&server, 0, sizeof server);
    server.sin_port = htons(atoi(argv[2]));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);

    char message_svr[1024];
    memset(message_svr, 0, sizeof message_svr);
    while (true)
    {
        std::cout << "client# ";
        std::getline(std::cin, message);
        if(message == "quit") break;
        // 当client首次发送消息给服务器的时候，OS会自动给client bind他的IP和PORT
        sendto(sock, message.c_str(), message.size(), 0, (struct sockaddr *)&server, sizeof server);   // server：给哪个ip/port发

        struct sockaddr_in temp;
        memset(&temp, 0, sizeof temp);
        socklen_t len = sizeof temp;
        ssize_t sz = recvfrom(sock, message_svr, sizeof message_svr - 1, 0, (struct sockaddr *)&temp, &len);  // temp：谁发给你的(哪个ip/port)
        if(sz > 0)
        {
            message_svr[sz] = 0;
            std::cout << "server echo# " << message_svr << std::endl;
        }
        else
        {
            // message_svr[0] = 0;
            // std::cout << "server echo# " << message_svr << std::endl;
        }
    }
    close(sock);
    return 0;
}