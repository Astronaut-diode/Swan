//
// Created by diode on 23-7-23.
// 更方便的调用写日志的方法，使用析构方法，将内容写入到日志文件中。

#ifndef SWAN_LOGSTREAM_H
#define SWAN_LOGSTREAM_H

#include <cstring>
#include <cassert>
#include <fstream>
#include <chrono>
#include <sstream>
#include "AsyncLog.h"

class LogStream {
public:  // 用于写typedef或者静态常量等。
    static const int kLogStreamMaxLength = 4 * 1024;  // 每一次写日志的最大内容。
private:  // 变量区域

public:

private:  // 函数区域
    char stream_[kLogStreamMaxLength];  // 日志内容。

public:
    LogStream(const char* fileName, int codeLine);

    LogStream &operator<<(const char *data);  // 追加日志内容。

    LogStream &operator<<(int data);  // 追加日志内容。

    LogStream &operator<<(const void* ptr);  // 追加日志内容。

    ~LogStream();  // 调用析构函数，并将内容写到AsyncLog的前端内存块中。
};

#define LOG LogStream(__FILE__, __LINE__)

#endif //SWAN_LOGSTREAM_H
