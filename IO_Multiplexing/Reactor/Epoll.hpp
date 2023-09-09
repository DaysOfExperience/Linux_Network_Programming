#pragma once
#include <sys/epoll.h>
#include <iostream>
#include <unistd.h>

class Epoll
{
    const static int gtimeout = 5000;  // 定时阻塞
private:
    int _epfd;
    int _timeout;
public:
    Epoll(int timeout = gtimeout)
    : _timeout(timeout)
    {}
    void Create()
    {
        _epfd = epoll_create(128);
        if(_epfd < 0) exit(5);
    }
    // 哪个文件描述符的哪些事件
    bool AddEvent(int sock, uint32_t events)
    {
        struct epoll_event ev;
        ev.data.fd = sock;
        ev.events = events;
        return 0 == epoll_ctl(_epfd, EPOLL_CTL_ADD, sock, &ev);
    }
    bool DelEvent(int sock)
    {
        return 0 == epoll_ctl(_epfd, EPOLL_CTL_DEL, sock, nullptr);
    }
    bool ModEvent(int sock, uint32_t events)
    {
        // 使sock在epoll中改为对events事件进行关心
        // 其实默认是读和ET
        struct epoll_event ev;
        ev.data.fd = sock;
        ev.events = events;
        return 0 == epoll_ctl(_epfd, EPOLL_CTL_MOD, sock, &ev);
    }
    int Wait(struct epoll_event *revs, int maxevents)
    {
        return epoll_wait(_epfd, revs, maxevents, _timeout);
    }
    ~Epoll()
    {
        if(_epfd >= 0)
            close(_epfd);
    }
};