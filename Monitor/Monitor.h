//
// Created by diode on 23-7-24.
//

#ifndef SWAN_MONITOR_H
#define SWAN_MONITOR_H

#include <cassert>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include "../Channel/Channel.h"
#include "../Poller/Poller.h"
#include "../Logger/LogStream.h"

class Monitor {
public:  // 用于写typedef或者静态常量等。
    static const int kThreadNameLength = 64;
private:  // 变量区域

public:
    int wakeupFd_;  // 用于唤醒使用的文件描述符。
    Channel *wakeupChannel_;  // 该文件描述符相关联的channel。
    Poller poller_;  // 该monitor关联的poller。
    char threadName_[kThreadNameLength];  // 线程的名字。

private:  // 函数区域

public:
    Monitor();

    Monitor(const char *threadName, int num = -1);

    ~Monitor();

    void handleWakeRead();  // 处理wakeupChannel的读回调事件。

    int createEventfd();  // 创建新的wakeup的文件描述符。

    void wakeup();  // 唤醒epoll_wait。

    void loop();  // 进行loop的死循环操作。
};

#endif //SWAN_MONITOR_H
