#pragma once

#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>

class Epoll
{
public:
    static int EpollCreate()
    {
        int epfd = epoll_create(256);
        if(epfd < 0)    exit(5);
        return epfd;
    }
    static bool EpollCtl(int epfd, int oper, int fd, uint32_t events)
    {
        struct epoll_event ev;
        ev.events = events;
        ev.data.fd = fd;
        return 0 == epoll_ctl(epfd, oper, fd, &ev);
    }
    //  int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
    static int EpollWait(int epfd, struct epoll_event *events, int maxevents, int timeout)
    {
        return epoll_wait(epfd, events, maxevents, timeout);
    }
};