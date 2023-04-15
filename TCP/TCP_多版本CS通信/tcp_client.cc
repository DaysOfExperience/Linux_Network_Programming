#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// 长连接版本
// ./tcp/client serverIp serverport
// int main(int argc, char **argv)
// {
//     if(argc != 3)
//     {
//         std::cout << "\nUsage: "<< argv[0] << " serverIp serverPort\n" << std::endl;
//         exit(1);
//     }
//     // 1. socket
//     int sock = socket(AF_INET, SOCK_STREAM, 0);
//     if(sock < 0)
//     {
//         std::cerr << "socket error" << std::endl;
//         exit(2);
//     }
//     // client 不需要显式地bind，但是一定需要port，让OS自动进行port选择
//     // 2. connect : 由客户端发起三次握手请求，对方处于listen状态!
//     struct sockaddr_in server;
//     memset(&server, 0, sizeof server);
//     server.sin_family = AF_INET;
//     server.sin_addr.s_addr = inet_addr(argv[1]);
//     server.sin_port = htons(atoi(argv[2]));
//     if(connect(sock, (struct sockaddr*)&server, sizeof server) < 0)
//     {
//         std::cerr << "connect error" << std::endl;
//         exit(3);
//     }
//     std::cout << "connect success" << std::endl;
//     // 3. 网络通信：read/write recv/send
//     char buff[1024];
//     while(true)
//     {
//         std::string line;
//         std::cout << "请输入# ";
//         std::getline(std::cin, line);
//         send(sock, line.c_str(), line.size(), 0);
//         ssize_t sz = recv(sock, buff, sizeof buff - 1, 0);
//         if(sz > 0)
//         {
//             buff[sz] = 0;
//             std::cout << "server# " << buff << std::endl;
//         }
//         else if(sz == 0)
//         {
//             // 对端关闭连接，我方需要close文件描述符，相当于发起第三次FIN挥手
//             std::cout << "server quit, clinet quit too" << std::endl;
//             break;
//         }
//         else
//         {
//             std::cerr << "recv error" << std::endl;
//             break;
//         }
//     }
//     close(sock);
//     return 0;
// }

// 短连接版

// ./tcp/client serverIp serverport
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cout << "\nUsage: " << argv[0] << " serverIp serverPort\n"
                  << std::endl;
        exit(1);
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof server);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(atoi(argv[2]));
    int sock = 0;
    char buff[1024];   // 获取server发来的处理结果
    std::string line;  // 获取用户输入
    while (true)
    {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        // std::cout << "sock : " << sock << std::endl;
        if (sock < 0)
        {
            std::cerr << "socket error" << std::endl;
            exit(2);
        }
        if (connect(sock, (struct sockaddr *)&server, sizeof server) < 0)
        {
            std::cerr << "connect error" << std::endl;
        }
        // std::cout << "connect success" << std::endl;
        // 3. 网络通信：read/write recv/send

        std::cout << "请输入# ";
        std::getline(std::cin, line);
        send(sock, line.c_str(), line.size(), 0);
        ssize_t sz = recv(sock, buff, sizeof buff - 1, 0);
        if (sz > 0)
        {
            buff[sz] = 0;
            std::cout << "server# " << buff << std::endl;
        }
        else if (sz == 0)
        {
            std::cout << "server quit, clinet quit too" << std::endl;
            break;
        }
        else
        {
            std::cerr << "recv error" << std::endl;
            break;
        }
        close(sock);
    }
    return 0;
}