#ifndef _TCP_SERVER_HPP_
#define _TCP_SERVER_HPP_

#include <iostream>
#include <string>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include "./threadpool/log.hpp"
#include "./threadpool/threadPool.hpp"

static void service(int sock, const std::string &client_ip, const uint16_t &client_port)
{
    // echo server
    // 注意，tcp是面向字节流的，可以直接使用文件IO接口: read write
    char buffer[1024];
    while (true)
    {
        ssize_t sz = read(sock, buffer, sizeof buffer);
        if (sz > 0)
        {
            buffer[sz] = 0;
            std::cout << client_ip << " : " << client_port << "# " << buffer << std::endl;
            write(sock, buffer, strlen(buffer)); // echo buffer，将发过来的数据当做字符串
        }
        else if (sz == 0)
        {
            // 有所不同，sz == 0（read返回0），表示客户端关闭连接（如果对方发送空字符串呢？... 事实证明，发送空串这里sz不会返回0，只针对于客户端关闭会返回0）
            logMessage(NORMAL, "%s:%d shutdown, service done", client_ip.c_str(), client_port);
            break;
        }
        else
        {
            logMessage(ERROR, "read socket error, %d:%s", errno, strerror(errno));
            break; // 读取失败，退出即可
        }
    }
    close(sock); // 注意，此处其实就是server从CLOSE_WAIT到LAST_ACK的状态转变过程，其实就是发送第三次FIN挥手
}

// 线程池版，就是多了一个线程名，标识哪个线程正在提供service
static void service_for_threadpool(int sock, const std::string &client_ip\
                    , const uint16_t &client_port, const std::string& name)
{
    // echo server
    // 注意，tcp是面向字节流的，可以直接使用文件IO接口: read write
    // 我们一般服务器进程业务处理，如果是从连上，到断开，要一直保持这个链接, 长连接。
    // 显然，长连接并不适合于线程池，因为线程池内线程数量有限   // 后面有其他方案！
    char buffer[1024];
    while(true)
    {
        ssize_t sz = read(sock, buffer, sizeof buffer);
        if(sz > 0)
        {
            buffer[sz] = 0;
            std::cout << name << " | " << client_ip << ":" << client_port << "# " << buffer << std::endl;
            write(sock, buffer, strlen(buffer));  // echo buffer，将发过来的数据当做字符串
        }
        else if(sz == 0)
        {
            // 有所不同，sz == 0，表示客户端关闭连接（如果对方发送空字符串呢？... 事实证明，发送空串这里sz不会返回0，只针对于客户端关闭会返回0）
            logMessage(NORMAL, "%s:%d shutdown, service done", client_ip.c_str(), client_port);
            break;
        }
        else
        {
            logMessage(ERROR, "read socket error, %d:%s", errno, strerror(errno));
            break;   // 读取失败，退出即可
        }
    }
    close(sock);
}

static void transform(int sock, const std::string &client_ip, const uint16_t &client_port, const std::string &name)
{
    char message[1024];
    ssize_t sz = recv(sock, message, sizeof message - 1, 0);
    if (sz > 0)
    {
        std::string result;
        message[sz] = '\0';
        char *ch = message;
        while (*ch != '\0')
        {
            if (islower(*ch) != 0)
                result += toupper(*ch);
            else
                result += *ch;
            ch++;
        }
        send(sock, result.c_str(), result.size(), 0);
    }
    else if (sz == 0)
    {
        logMessage(NORMAL, "%s:%d shutdown, service done", client_ip.c_str(), client_port);
    }
    else
    {
        logMessage(ERROR, "service recv error");
    }
    close(sock);  // 服务端进行第三次挥手！不会出现大量CLOSE_WAIT连接
}

// class ThreadData
// {
// public:
//     ThreadData() = default;
//     ThreadData(int sock, const std::string &ip, uint16_t port)
//         : _sock(sock), _ip(ip), _port(port)
//     {
//     }
//     int _sock;
//     std::string _ip;
//     uint16_t _port;
// };

class TcpServer
{
private:
    const static int gback_log = 20;
    // static void *threadRoutine(void *args)
    // {
    //     pthread_detach(pthread_self()); // 线程分离
    //     ThreadData *td = (ThreadData *)args;
    //     service(td->_sock, td->_ip, td->_port); // 让子线程给客户端提供服务
    //     delete td;                              // ThreadData动态开辟..
    //     // close(td->_sock);                       // 第三次FIN挥手...需要，但是其实service里面已经close过了。
    //     return nullptr;
    // }

public:
    TcpServer(uint16_t port, const std::string &ip = "")
    : _port(port), _ip(ip), _listen_sock(-1), _threadpool_ptr(ThreadPool<Task>::getThreadPool(5))  // 单例模式..
    {}
    // TcpServer(uint16_t port, const std::string &ip = "")
    //     : _port(port), _ip(ip), _listen_sock(-1)
    // {
    // }
    void initServer()
    {
        // 1. 创建套接字
        _listen_sock = socket(AF_INET, SOCK_STREAM, 0); // 网络套接字, 面向字节流（tcp）
        if (_listen_sock < 0)
        {
            logMessage(FATAL, "create listen socket error, %d:%s", errno, strerror(errno));
            exit(2);
        }
        logMessage(NORMAL, "create listen socket success: %d", _listen_sock);

        // 2. bind，注意云服务器不能绑定公网IP，不允许。
        struct sockaddr_in local;
        memset(&local, 0, sizeof local);
        local.sin_family = AF_INET;
        local.sin_port = htons(_port);
        local.sin_addr.s_addr = _ip.empty() ? INADDR_ANY : inet_addr(_ip.c_str());
        if (bind(_listen_sock, (struct sockaddr *)&local, sizeof local) < 0)
        {
            logMessage(FATAL, "bind error, %d:%s", errno, strerror(errno));
            exit(3);
        }

        // 3. listen监听: 因为TCP是面向连接的，在我们正式通信之前，需要先建立TCP连接
        // listen: 将套接字状态设置为监听状态。服务器要一直处于监听状态，这样客户端才能随时发起连接。
        if (listen(_listen_sock, gback_log) < 0) // gback_log后面讲，全连接队列的长度。我猜测就是这个服务器同一时刻允许连接的客户端的数量最大值？也不太对呀，这么少么？
        {
            logMessage(FATAL, "listen error, %d:%s", errno, strerror(errno));
            exit(4);
        }
        logMessage(NORMAL, "init server success");
    }
    void start()
    {
        // signal(SIGCHLD, SIG_IGN); // 让子进程执行完service之后自动回收自己的资源，避免僵尸进程。多进程版1需要
        _threadpool_ptr->run(); // 线程池内所有线程开始执行routine，等待任务投放
        while (true)
        {
            // 4. 获取与客户端建立好的连接（注意，accept不进行三次握手）
            struct sockaddr_in client;
            socklen_t len = sizeof client;
            // 注意，这里要放在循环内部，因为要获取与多个客户端建立好的连接（要获取与多个客户端建立好的连接）
            int service_sock = accept(_listen_sock, (struct sockaddr *)&client, &len); // 返回一个用于与客户端进行网络IO的套接字，不同于listen_sock
            // On success, these system calls return a nonnegative integer that is a descriptor for the accepted socket.  On error, -1 is returned, and errno is set appropriately.
            if (service_sock < 0)
            {
                logMessage(ERROR, "accept error, %d:%s", errno, strerror(errno));
                continue; // 继续获取新连接，并非FATAL的
            }
            // 5. 获取连接成功，开始进行网络通信

            uint16_t client_port = ntohs(client.sin_port);      // 连接的客户端的port
            std::string client_ip = inet_ntoa(client.sin_addr); // 连接的客户端的ip
            logMessage(NORMAL, "get link(accept) success, service socket: %d | %s : %d", service_sock,
                       client_ip.c_str(), client_port);

            // 真·获取连接成功，开始进行网络通信
            {
                // // version 1: 单进程循环版，server端进程直接提供service版
                // // 同一时刻只能给一个client提供服务，处理完一个，service退出，才能再进行accept，显然不行。
                // 注意，这里其他客户端可以连接，因为listen了，只是主进程没有进行accept!
                // service(service_sock, client_ip, client_port);
            }
            {
                // // version 2: 多进程：父进程创建子进程，让子进程给客户端提供服务。父进程去再次accept获取新的客户端连接。
                // // 这样做的一个条件原理是：子进程fork出来之后自动继承父进程的文件描述符表
                // pid_t pid = fork();
                // assert(pid != -1);
                // if(pid == 0)
                // {
                //     // 子进程
                //     close(_listen_sock);  // _listen_sock是父进程用于accept与客户端获取连接的，子进程不需要。
                //     service(service_sock, client_ip, client_port);
                //     exit(0);  // 子进程给客户端提供完服务，退出。
                // }
                // close(service_sock);  // 父进程关闭service_sock（通常为4）否则随着客户端数量增多，文件描述符资源将耗尽
                //                       // 文件描述符泄漏。文件描述符本质是数组下标，是数组，则就有空间上限。
                //                       // 注意，此处为server从CLOSE_WAIT到LAST_ACK的状态转变过程，其实就是发送第三次FIN挥手。
            }
            {
                // // version 2.1 多进程2.0
                // // 创建子进程，让子进程再创建子进程去给客户端提供service，父进程继续while循环accept
                // // 其实只是回收子进程，避免僵尸进程的另一种方法罢了。
                // pid_t pid = fork();
                // if(pid == 0)
                // {
                //     // 子进程
                //     if(fork() > 0) exit(0); // 子进程直接退出
                //     // 孙子进程，它的父进程上面已经exit了，变为孤儿进程，由OS自动回收孤儿进程（让1号进程领养）
                //     close(_listen_sock);
                //     service(service_sock, client_ip, client_port);
                //     exit(0);
                // }
                // // 父进程
                // // close(service_sock);  // 依旧，这里为server进行第三次FIN挥手。避免大量CLOSE_WAIT连接
                // waitpid(pid, nullptr, 0);  // 虽然是0（阻塞式等待），但是不会阻塞。
            }
            {
                // // 多线程版本
                // pthread_t tid;
                // ThreadData *td = new ThreadData(service_sock, client_ip, client_port); // 这里不能在栈区开辟ThreadData
                // // ThreadData td(service_sock, client_ip, client_port);
                // pthread_create(&tid, nullptr, threadRoutine, (void *)td);
                // // 在多线程这里不用进程关闭特定的文件描述符 service_sock，线程会关闭的，也就是threadRoutine内部
            }
            {
                // 线程池版本
                // Task task(service_sock, client_ip, client_port, service_for_threadpool); // 长链接，不适合线程池版本

                // 客户端socket，connect，服务端accept。服务端的主线程往线程池的任务队列内放入一个Task，主线程继续去accept
                // 某个线程被pthread_cond_siganl，执行transform，客户端从键盘输入，send，服务端recv，处理数据，send，客户端recv。这里的recv是会阻塞对方的。
                // 客户端最后recv之后，服务端这个线程的Task执行结束，继续去检测任务队列，pthread_cond_wait
                // 客户端也close当前这个sock，客户端再次socket，connect时，服务端的主线程早就在accept等待连接了。
                Task task(service_sock, client_ip, client_port, transform);  // 指派线程池内某个线程执行transform
                _threadpool_ptr->pushTask(task);
            }
        }
    }
    ~TcpServer()
    {
        close(_listen_sock);
    }
private:
    uint16_t _port;   // 端口号
    std::string _ip;  // ip
    int _listen_sock; // 监听套接字，listen创建，用于accept
    ThreadPool<Task> *_threadpool_ptr;
};

#endif