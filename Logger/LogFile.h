//
// Created by diode on 23-7-23.
// 日志文件类，将打开的目标文件形式化为类。

#ifndef SWAN_LOGFILE_H
#define SWAN_LOGFILE_H

#include <sys/time.h>
#include <iostream>
#include <cstring>
#include <cassert>
#include <fstream>
#include <unistd.h>
#include <chrono>
#include <ctime>

class LogFile {
public:  // 用于写typedef或者静态常量等。
    static const int kLogDirPathLength = 256;  // 日志文件夹路径的最长长度。
    static const int kLogFileNameLength = 128;  // 日志文件夹路径的最长长度。
    static const int kMaxSize = 40 * 1024 * 1024;  // 日志文件的最大大小为40M。
    static const int kMaxInterval = 10 * 60;  // 日志文件的最大时间间隔为10分钟。
private:  // 变量区域
    bool writeCout_;  // 是否是使用cout输出。
    char logDirPath_[kLogDirPathLength];  // 日志文件夹的路径。
    char logFileName_[kLogFileNameLength];  // 日志文件的名字。
    int size_;  // 写入了字节数量，抵达一定量以后会关闭当前文件，创建新的文件。
    time_t lastTime_;  // 最后一次的时间。
    std::ofstream out_;  // 使用的流，如果是终端输出，直接给定cout，否则给定文件流。
public:

private:  // 函数区域

public:
    LogFile() {};

    LogFile(bool outputTerminal, const char *logDirPath);

    ~LogFile();

    void reset();  // 重新打开一个文件描述符，并设置新的日志文件的名字。

    void append(const char *data, int size);  // 往目标的流中追加内容。
};

#endif //SWAN_LOGFILE_H
