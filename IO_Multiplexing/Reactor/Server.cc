#include "TcpServer.hpp"
#include <memory>
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
void NetCal(Connection *conn, const std::string &request)
{
    // 某一个client的TCP连接发来一个应用层报文（序列化+报头）
    // 这一定是一个合法的应用层报文
    std::string package = deCode(request);  // 去报头
    // 有效载荷在package中。如"1 + 2"
    Request req;
    req.deserialize(package);
    Response resp = calculatorHelp(req);
    std::string backStr = resp.serialize();  // 响应序列化
    backStr = enCode(backStr);  // 序列化的响应添加报头
    conn->_out_buffer += backStr;  // 有响应报文就绪，在发送缓冲区中
    conn->_server_ptr->EnableRW(conn, true, true);
}

int main()
{
    std::unique_ptr<TcpServer> ts(new TcpServer());
    ts->Dispather(NetCal);
    return 0;
}