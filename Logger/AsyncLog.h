//
// Created by diode on 23-7-22.
// 异步日志类的头文件，负责管理记录异步日志的线程，并实现双缓冲方法。

#ifndef SWAN_ASYNCLOG_H
#define SWAN_ASYNCLOG_H

#include <vector>
#include <memory>
#include <unistd.h>
#include <cassert>
#include "FixedMemBlock.h"
#include <sys/time.h>
#include "LogFile.h"
#include "../Thread/Thread.h"
#include "../Utils/Utils.h"

class AsyncLog {
public:  // 用于写typedef或者静态常量等。
    static const int kMemBlockSize = 4 * 1024 * 1024;  // 内存块的大小，是4M。
    static const int kLogFilePathLength = 256;  // 日志文件夹路径的最长长度。
    static const int kMaxInterval_ = 3;  // 条件变量等待的最长时间。
    const char *kLogDirPath = "/Other/Log\0";  // 日志文件夹的路径位置。
    const char *kLogThreadName = "Log\0";  // 开启日志异步功能的时候，日志线程的名字。
    typedef FixedMemBlock<kMemBlockSize> Buffer;
    typedef std::vector<std::unique_ptr<Buffer>> BufferVec;
    typedef BufferVec::value_type BufferPtr;
private:  // 变量区域
    bool outputTerminal_;  // 是否输出到终端，即日志写到文件中还是日志文件里面。
    BufferPtr currentPtr_;  // 当前正在使用的内存块。
    BufferPtr nextPtr_;  // 如果当前使用的内存块用完了，那就改用这一块，空闲时间开辟，可以减少忙碌时间的等待行为。
    BufferVec buffers_;  // 每一个currentPtr_写满以后都得放到这里面来。
    char logDirPath_[kLogFilePathLength];  // 日志文件夹的路径。
    std::unique_ptr<Thread> thread_;  // 对应的异步子线程,使用unique进行生命周期的管理工作。
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
    struct timespec next_;  // 超时等待的时间。
    std::unique_ptr<LogFile> logFile_;  // 待写入的日志文件。
public:

private:  // 函数区域
    void ThreadCallBack();  // 异步子线程需要执行的工作。

public:
    AsyncLog();

    explicit AsyncLog(bool outputTerminal);

    ~AsyncLog();

    void appendToCurrent(const char *data, int size);
};

#endif //SWAN_ASYNCLOG_H
