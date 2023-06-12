#include "http_server.hpp"
#include "util.hpp"
#include "daemon.hpp"
#include <fstream>
#include <memory>
#include <vector>
#include <string>

// 一般http都要有自己的web根目录
#define ROOT "./wwwroot" // ./wwwroot/index.html
// 如果客户端只请求了一个/,我们返回默认首页
#define HOMEPAGE "./wwwroot/index.html"   // 默认首页。

// 短链接，http协议本质就是文本分析。
void HttpServerHandler(int sock)
{
    char buffer[10240];
    ssize_t sz = recv(sock, buffer, sizeof buffer, 0);
    if (sz > 0)
    {
        buffer[sz] = '\0';
    }
    std::cout << "---------------------------------" << std::endl;
    std::cout << buffer << std::endl;
    std::cout << "---------------------------------" << std::endl;

    std::vector<std::string> vline;
    Util::cutString(buffer, "\r\n", &vline); // 每一行有了

    std::vector<std::string> vfirst;
    Util::cutString(vline[0], " ", &vfirst);
    
    std::string file = vfirst[1]; // "/" "/a/b/c/hh.html"   url，统一资源描述符。中，域名（ip地址）之后的内容。
    std::string target = ROOT;    //
    if (file == "/")
        target = HOMEPAGE;
    else
        target += file;

    // 提取target文件的内容？目前这里不太懂，fstream的用法。
    std::string content;
    std::ifstream in(target);
    if (in.is_open())
    {
        std::string line;
        while (std::getline(in, line))
        {
            content += line;
        }
        in.close();
    }

    std::string HttpResponse;
    if (content.empty())  // 目标资源不存在或者资源文件内部为空
    {
        // HttpResponse = "Http/1.1 302 Found\r\n";  // 状态行，发现：这里302还是404是有区别的。事实证明，404和Location不能配合
        // HttpResponse += "Location: http://101.34.65.169:8888/a/b/404.html";   // 响应报头，也就是如果想访问的资源不存在，则跳转至这个页面
        HttpResponse = "HTTP/1.1 404 NotFound\r\n";
    }
    else
        HttpResponse = "HTTP/1.1 200 OK\r\n";
    HttpResponse += "\r\n";  // 此处为空行，响应报头省略掉
    HttpResponse += content; // 响应正文
    send(sock, HttpResponse.c_str(), HttpResponse.size(), 0);
}
// 哈哈啊
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "\nUsage: " << argv[0] << " port\n"
                  << std::endl;
        exit(1);
    }
    // MyDaemon();
    std::unique_ptr<HttpServer> server(new HttpServer(atoi(argv[1]), HttpServerHandler));
    server->start();
    return 0;
}