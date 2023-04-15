#pragma once
#include <functional>
#include <string>
#include "log.hpp"

typedef std::function<void (int, const std::string &, const uint16_t &, const std::string&)> fun_t; 

class Task
{
public:
    Task() = default;
    Task(int sock, const std::string& ip, uint16_t port, fun_t func)
    : _sock(sock), _ip(ip), _port(port), func_(func)
    {}
    void operator()(const std::string& name)  // 线程池内线程执行这个operator()
    {
        std::cout << name << " working for " << _ip << ":" << _port << std::endl;
        func_(_sock, _ip, _port, name);
        // logMessage(NORMAL, "%s处理完成: %d+%d=%d | %s | %d", name.c_str() , x_, y_, func_(x_, y_), __FILE__, __LINE__);
    }
public:
    int _sock;
    std::string _ip;
    uint16_t _port;
    fun_t func_;
};