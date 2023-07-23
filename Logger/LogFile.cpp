//
// Created by diode on 23-7-23.
//

#include "LogFile.h"

LogFile::LogFile(bool outputTerminal, const char *logDirPath) : writeCout_(outputTerminal) {
    strcpy(logDirPath_, logDirPath);
    if (!writeCout_) {  // 如果要输出到文件中，那就要构造文件的名字，并打开文件描述符。
        reset();
    }
}

LogFile::~LogFile() {
    if(!writeCout_) {
        if(out_.is_open()) {
            out_.close();
        }
    }
}

/**
 * 重新打开一个文件描述符，并设置新的日志文件的名字。
 */
void LogFile::reset() {
    assert(!writeCout_);
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();  // 获取当前时间点
    lastTime_ = std::chrono::system_clock::to_time_t(now);  // 将时间点转换为本地时间
    std::tm localTime = *std::localtime(&lastTime_);
    // 获取年、月、日、时、分、秒
    int year = localTime.tm_year + 1900; // 年份是从1900年开始的
    int month = localTime.tm_mon + 1;    // 月份是从0开始的，所以要加1
    int day = localTime.tm_mday;
    int hour = localTime.tm_hour;
    int minute = localTime.tm_min;
    int second = localTime.tm_sec;
    snprintf(logFileName_, kLogFileNameLength, "%d-%d-%d %d:%d:%d", year, month, day, hour, minute, second);  // 创建文件名字。
    if (out_.is_open()) {  // 先确保是关闭状态。
        out_.close();
    }
    char bufferFilePath[kLogDirPathLength + kLogFileNameLength]{'\0'};
    strcat(bufferFilePath, logDirPath_);
    bufferFilePath[strlen(bufferFilePath)] = '/';
    strcat(bufferFilePath, logFileName_);  // 构建出完整的路径。
    out_ = std::ofstream(bufferFilePath, std::ios::app);  // 打开对应的路径。
    assert(out_.is_open());
    size_ = 0;
}

/**
 * 往文件中输入内容，而且不会增加任何附带的内容。
 * @param data
 * @param size
 */
void LogFile::append(const char *data, int size) {
    if(writeCout_) {
        std::cout << data;
    } else {
        out_ << data;
        out_.flush();  // 通常这是一个内存块，直接刷新掉就行。
        size_ = size_ + size;
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();  // 获取当前时间点
        time_t time = std::chrono::system_clock::to_time_t(now);
        if(size_ > kMaxSize || time - lastTime_ > kMaxInterval) {  // 如果内容超出了限制或者时间太久了，都要改换文件。
            reset();
        }
    }
}