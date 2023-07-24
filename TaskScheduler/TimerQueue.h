//
// Created by diode on 23-7-23.
// 定时器队列，会将每一个定时器汇总到这里，并使用新建立的线程，在其中创建对应的Epoll监听事件。

#ifndef SWAN_TIMERQUEUE_H
#define SWAN_TIMERQUEUE_H

#include <ctime>
#include <sys/timerfd.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <set>
#include "Timer.h"
#include "../Channel/Channel.h"
#include "../Poller/Poller.h"
#include "../Logger/LogStream.h"

typedef std::pair<long long, Timer *> TimerEntry;  // 定义一个pair对，超时时间-定时器。
typedef std::set<TimerEntry> TimerList;  // 因为使用的是TimerEntry,所以底层排序的时候会按照第一个元素进行排。

class TimerQueue {
public:  // 用于写typedef或者静态常量等。

private:  // 变量区域
    int timerFd_;  // 计时器的文件描述符。
    TimerList timers_;  // 所有的定时器组成的红黑树，第一个元素就是最先超时的。
    Channel *timeChannel_;
    Poller poller_;
public:

private:  // 函数区域
    int createTimerFd();  // 创建时间的文件描述符。

    bool insert(Timer *timer, timeval expire);  // 往timers_中插入一个新的定时器。

    void resetTimerfd(timeval expire);  // 重新设置监听的超时时间。
public:
    TimerQueue();

    ~TimerQueue();

    timeval addTimer(FunctionCallback cb,
                     double interval,
                     bool repeat);  // 向定时器列表中增加一个定时器,并返回超时的时间。

    int pollerFd();  // 返回pollerFd。

    Poller& getPoller();

    void processRead();

    std::vector<TimerEntry> getExpired();  // 找出所有的过期定时器。

    void reset(std::vector<TimerEntry> expired);  // 检查超时的内容是否需要重新插入回去。
};

#endif //SWAN_TIMERQUEUE_H
