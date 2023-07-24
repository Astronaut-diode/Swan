//
// Created by diode on 23-7-23.
// Epoll类。

#ifndef SWAN_POLLER_H
#define SWAN_POLLER_H

#include <sys/epoll.h>
#include <vector>
#include <cassert>
#include <cstring>
#include <memory>
#include "../Channel/Channel.h"


typedef std::vector<struct epoll_event> EventList;

class Poller {
public:  // 用于写typedef或者静态常量等。
    static const int kEventListInitLength = 16;  // event事件列表的初始长度。
private:  // 变量区域
    int pollerFd_;  // 监听的文件描述符。
    std::unique_ptr<EventList> eventList_;  // 需要监听的数组，并用于接收结果的数组。

public:
private:  // 函数区域

public:
    Poller();

    ~Poller();

    void updateEpollEvents(int operation, Channel *channel);  // 更新channel的关心事件。

    int pollerFd();

    EventList &getEventList();
};

#endif //SWAN_POLLER_H
