//
// Created by diode on 23-7-23.
//

#include "LogStream.h"

extern AsyncLog asyncLog;

LogStream::LogStream(const char *fileName, int codeLine) {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();  // 获取当前时间点
    time_t time = std::chrono::system_clock::to_time_t(now);  // 将时间点转换为本地时间
    std::tm localTime = *std::localtime(&time);
    // 获取年、月、日、时、分、秒
    int year = localTime.tm_year + 1900; // 年份是从1900年开始的
    int month = localTime.tm_mon + 1;    // 月份是从0开始的，所以要加1
    int day = localTime.tm_mday;
    int hour = localTime.tm_hour;
    int minute = localTime.tm_min;
    int second = localTime.tm_sec;
    char pthreadName[16];  // 获取当前线程的名字
    pthread_getname_np(pthread_self(), pthreadName, sizeof(pthreadName));
    snprintf(stream_, kLogStreamMaxLength, "[%d-%2d-%2d %2d:%2d:%2d %s:%s:%4d] ", year, month, day, hour, minute,
             second,
             pthreadName, fileName, codeLine);  // 设置日志的每一行打头的内容。
    count_ = strlen(stream_);
}

/**
 * 往Logtream中追加内容。
 * @param data
 * @return
 */
LogStream &LogStream::operator<<(const char *data) {
    strncat(stream_, data, std::min(static_cast<int>(strlen(data)), kLogStreamMaxLength - count_));
    count_ += std::min(static_cast<int>(strlen(data)), kLogStreamMaxLength - count_);  // 解决日志太长的问题。
    return *this;
}

// 重载 << 运算符模板，用于输出指针的地址
LogStream &LogStream::operator<<(const void *ptr) {
    std::ostringstream oss;
    oss << ptr;
    strncat(stream_, oss.str().c_str(), std::min(static_cast<int>(strlen(oss.str().c_str())), kLogStreamMaxLength - count_));
    count_ += std::min(static_cast<int>(strlen(oss.str().c_str())), kLogStreamMaxLength - count_);  // 解决日志太长的问题。
    return *this;
}

/**
 * 追加日志内容。
 * @param data
 * @return
 */
LogStream &LogStream::operator<<(int data) {
    return (*this) << std::to_string(data).c_str();
}

/**
 * 将内容都写入到AsyncLog的前端内存块中。
 */
LogStream::~LogStream() {
    asyncLog.appendToCurrent(stream_, strlen(stream_));
}
