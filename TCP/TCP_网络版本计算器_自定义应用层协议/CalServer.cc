#include "TcpServer.hpp"
#include "Protocol.hpp"
#include <memory>

using namespace ns_tcpserver;
using namespace ns_protocol;

Response calculatorHelp(const Request &req)
{
    // "1+1"???
    Response resp;
    int x = req._x;
    int y = req._y;
    switch (req._op)
    {
    case '+':
        resp._result = x + y;
        break;
    case '-':
        resp._result = x - y;
        break;
    case '*':
        resp._result = x * y;
        break;
    case '/':
        if (y == 0)
            resp._code = 1;
        else
            resp._result = x / y;
        break;
    case '%':
        if (y == 0)
            resp._code = 2;
        else
            resp._result = x % y;
        break;
    default:
        resp._code = 3;
        break;
    }
    return resp;
}

void calculator(int sock)
{
    std::string s;
    for (;;)
    {
        if (!Recv(sock, &s)) // 输出型参数
            break;  // 大概率对端退出，则服务结束。一般不会读取失败recv error
        std::string package = deCode(s);
        if (package.empty())
            continue; // 不是一个完整报文，继续读取
        // 读取到一个完整报文，且已经去了应用层报头，有效载荷在package中。如"1 + 2"
        Request req;
        req.deserialize(package);
        Response resp = calculatorHelp(req);
        std::string backStr = resp.serialize();
        backStr = enCode(backStr);
        if (!Send(sock, backStr)) // 发送失败就退出
            break;
    }
}

// ./cal_server port
int main(int argc, char **argv)
{
    // std::cout << "test remake" << std::endl;  // success
    if (argc != 2)
    {
        std::cout << "\nUsage: " << argv[0] << " port\n"
                  << std::endl;
        exit(1);
    }
    std::unique_ptr<TcpServer> server(new TcpServer(atoi(argv[1])));
    server->bindService(calculator); // 给服务器设置服务方法，将网络服务和业务逻辑进行解耦
    server->start();                 // 服务器开始进行accept，连接一个client之后就提供上方bind的服务
    return 0;
}