// 一个打印，提示的功能
#pragma once
#include <iostream>
#include <cstdio>
#include <cstdarg>

// 日志是有级别的
#define DEBUG 0
#define NORMAL 1
#define WARNING 2
#define ERROR 3
#define FATAL 4

const char *gLevelMap[] = {
    "DEBUG",
    "NORMAL",
    "WARNING",
    "ERROR",
    "FATAL"
};

// 第二个参数本质上就是一个字符串，你忘了吗 "你好，哈哈，%s%d"
void logMessage(int level, const char *format, ...)
{
//     // 如果没有定义DEBUG_SHOW，你就把DEBUG level的log都屏蔽掉
// #ifndef DEBUG_SHOW
//     if(level == DEBUG)  return;
// #endif
    char stdBuffer[1024];   // 标准部分的存储空间
    time_t timestamp = time(nullptr);   // 获取时间戳以代替时间
    snprintf(stdBuffer, sizeof stdBuffer, "[%s] [%d] ", gLevelMap[level], timestamp);

    char logBuffer[1024];   // 自定义部分的存储空间
    va_list args;
    va_start(args, format);
    vsnprintf(logBuffer, sizeof logBuffer, format, args);
    va_end(args);

    printf("%s%s\n", stdBuffer, logBuffer);
}