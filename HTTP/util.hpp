#pragma once

#include <vector>

class Util
{
public:
    static void cutString(const std::string &s, const std::string &sep, std::vector<std::string> *v)
    {
        // 对s字符串进行切分，以sep为切割符，切分完成的字符串放入v中
        // "aaaaaa\r\nbbbbbbb\r\ncccccccc"
        std::size_t start = 0;
        while(start < s.size())
        {
            std::size_t pos = s.find(sep, start);
            if(pos == std::string::npos)    break;
            v->push_back(s.substr(start, pos - start));  // 注意substr的第二个参数是切割的字符数量。
            start = pos + sep.size();
        }
        if(start < s.size())
            v->push_back(s.substr(start));
    }
};