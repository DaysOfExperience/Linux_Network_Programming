#include "Protocol.hpp"
#include "Sock.hpp"
#include <memory>

using namespace ns_protocol;

// ./client serverIp serverPort
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cout << "\nUsage: " << argv[0] << " serverIp serverPort\n"
                  << std::endl;
        exit(1);
    }
    Sock sock;
    int sockfd = sock.Socket();
    // 客户端不需要显式bind, 老生常谈了。
    if (sock.Connect(sockfd, argv[1], atoi(argv[2])) == -1)
    {
        std::cout << "connect error" << std::endl;
        exit(3);
    }
    std::string backStr;
    bool quit = false;
    while (!quit)
    {
        Request req;
        std::cout << "Please enter# ";
        std::cin >> req._x >> req._op >> req._y;
        std::string reqStr = req.serialize();
        reqStr = enCode(reqStr); // 添加长度报头，此处添加报头（制定协议）是为了解决TCP粘包问题，因为TCP是面向字节流的。
        if (!Send(sockfd, reqStr))
            break;
        while (true)
        {
            if (!Recv(sockfd, &backStr))
            {
                quit = true;
                break;
            }
            std::string package = deCode(backStr);   // 服务端发来的报文在backStr中，可能粘包
            if(package.empty())
                continue; // 这次不是一个完整的应用层报文，继续读取
            // 读取到一个完整的应用层报文，且已经去报头，获取有效载荷成功，在package中。
            Response resp;
            resp.deserialize(package);
            switch (resp._code)
            {
            case 1:
                std::cout << "除零错误" << std::endl;
                break;
            case 2:
                std::cout << "模零错误" << std::endl;
                break;
            case 3:
                std::cout << "其他错误" << std::endl;
                break;
            default:
                std::cout << req._x << " " << req._op << " " << req._y << " = " << resp._result << std::endl;
                break;
            }
            break;
        }
    }
    close(sockfd);
    return 0;
}
