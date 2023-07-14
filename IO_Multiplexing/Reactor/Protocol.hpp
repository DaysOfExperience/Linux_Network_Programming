#ifndef _PROTOCOL_HPP_
#define _PROTOCOL_HPP_

#include <iostream>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <jsoncpp/json/json.h>

// important and new
namespace ns_protocol
{

// #define MYSELF 1 // 自己实现序列化反序列化还是使用json库

// SPACE用于序列化和反序列化
#define SPACE " "
#define SPACE_LENGTH strlen(SPACE)

#define SEP "\r\n"
#define SEP_LENGTH strlen(SEP)

    // 请求和回复，都需要序列化和反序列化的成员函数
    // 序列化和反序列化双方都不同。但是添加报头和去报头是相同的，"Length\r\nxxxxx\r\n";
    // 客户端生成请求，序列化之后发送给服务端
    class Request
    {
    public:
        Request() = default;
        Request(int x, int y, char op)
            : _x(x), _y(y), _op(op)
        {
        }
        ~Request() {}

    public:
        int _x;
        int _y;
        char _op;

    public:
        std::string serialize()
        {
            // 序列化为"_x _op _y"  （注意，序列化和添加报头是分开的，反序列化和去掉报头是分开的
            std::string s = std::to_string(_x);
            s += SPACE;
            s += _op;
            s += SPACE;
            s += std::to_string(_y);
            return s;
        }
        bool deserialize(const std::string &s)
        {
            // "_x _op _y"
            std::size_t left = s.find(SPACE);
            if (left == std::string::npos)
                return false;
            std::size_t right = s.rfind(SPACE);
            if (right == left)
                return false;
            _x = atoi(s.substr(0, left).c_str());
            _op = s[left + SPACE_LENGTH];
            _y = atoi(s.substr(right + SPACE_LENGTH).c_str());
            return true;
        }
    };
    // 服务端收到请求，反序列化，业务处理生成response，序列化后发送给客户端
    class Response
    {
    public:
        Response(int result = 0, int code = 0)
            : _result(result), _code(code)
        {
        }
        ~Response() {}

    public:
        std::string serialize()
        {
            // 序列化为"_code _result"  （注意，序列化和添加报头是分开的，反序列化和去掉报头是分开的
            std::string s = std::to_string(_code);
            s += SPACE;
            s += std::to_string(_result);
            return s;
        }
        bool deserialize(const std::string &s)
        {
            // "_code _result"
            std::size_t pos = s.find(SPACE);
            if (pos == std::string::npos)
                return false;
            _code = atoi(s.substr(0, pos).c_str());
            _result = atoi(s.substr(pos + SPACE_LENGTH).c_str());
            return true;
        }
    public:
        int _result;
        int _code; // 状态码
    };
    
    // 进行去报头，报文完整则去报头，并返回有效载荷，不完整则代表失败返回空字符串。
    std::string deCode(std::string &s) // 输入型输出型参数
    {
        // "Length\r\nx op y\r\n"   成功返回有效载荷，失败返回空串
        std::size_t left = s.find(SEP);
        if (left == std::string::npos)
            return "";
        std::size_t right = s.rfind(SEP);
        if (right == left)
            return "";
        int length = atoi(s.substr(0, left).c_str());
        if (length > s.size() - left - 2 * SEP_LENGTH)
            return ""; // 有效载荷长度不足，不是一个完整报文，其实经过上面两次的if判断已经够了可能。
        // 是一个完整报文，进行提取
        std::string ret;
        s.erase(0, left + SEP_LENGTH);
        ret = s.substr(0, length);
        s.erase(0, length + SEP_LENGTH);
        return ret;
    }
    std::string enCode(const std::string &s)
    {
        // "Length\r\n1+1\r\n"
        std::string retStr = std::to_string(s.size());
        retStr += SEP;
        retStr += s;
        retStr += SEP;
        return retStr;
    }
    // 我真的很想用引用，但是好像传统规则是输出型参数用指针...
    // 其实这个Recv就是一个单纯的读数据的函数，将接收缓冲区数据读到应用层缓冲区中，也就是*s中。存储的是对端发来的应用层报文。
    bool Recv(int sock, std::string *s)
    {
        // 仅仅读取数据到*s中
        char buff[1024];
        ssize_t sz = recv(sock, buff, sizeof buff, 0);
        if (sz > 0)
        {
            buff[sz] = '\0';
            *s += buff;    
            return true;
        }
        else if (sz == 0)
        {
            std::cout << "peer quit" << std::endl;
            return false;
        }
        else
        {
            std::cout << "recv error" << std::endl;
            return false;
        }
    }
    bool Send(int sock, const std::string &s)
    {
        ssize_t sz = send(sock, s.c_str(), s.size(), 0);
        if (sz > 0)
        {
            return true;
        }
        else
        {
            std::cout << "send error!" << std::endl;
            return false;
        }
    }
}

#endif