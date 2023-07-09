// 非阻塞式IO
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

// 利用fcntl将指定文件描述符设置为非阻塞
bool SetNonBlock(int fd)
{
    int fl = fcntl(fd, F_GETFL); // 底层获取该fd对应的文件状态标记位(通过返回值获取)
    if(fl < 0)
        return false;
    return -1 != fcntl(fd, F_SETFL, fl | O_NONBLOCK); // 设置非阻塞，在原有的文件标记位上添加一个非阻塞标记位
}

int main()
{
    SetNonBlock(0); // 将标准输入设置为非阻塞（默认为阻塞）
    
    // 非阻塞IO，也就是读或者写的条件没有就绪时（没数据或没空间），此时不进行阻塞
    // 而条件不就绪具体是以出错的形式返回的，从而告知上层条件没有就绪
    // 所以，当读取或写入出错时（下方是以读取为例的），要判断是条件没有就绪从而出错，还是其他类型的出错
    char buff[1024];
    while(true)
    {
        sleep(3);
        errno = 0;
        ssize_t sz = read(0, buff, sizeof buff - 1);
        if(sz > 0)
        {
            buff[sz-1] = 0;   // 将我们输入的换行符覆盖为'\0'
            std::cout << "标准输入：" << buff << " errno: " << errno << " errstring: " << strerror(errno) << std::endl;
        }
        else
        {
            std::cout << "read \"error\", errno:" << errno << " errstring:" << strerror(errno) << std::endl;
            if(errno == EWOULDBLOCK || errno == EAGAIN)
            {
                std::cout << "当前0号文件描述符的数据没有就绪(没有可读的数据)" << std::endl;
                continue;  // 非阻塞式：进行下一次读取
            }
            else if(errno == EINTR)
            {
                std::cout << "当前IO被信号中断,请再次读取" << std::endl;
                continue;
            }
            else
            {
                // 没有读取成功，且不是因为没有数据，也不是因为信号。故出现了错误
                // 进行差错处理
                // ...
            }
        }
    }
    return 0;
}