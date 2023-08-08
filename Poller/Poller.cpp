//
// Created by diode on 23-7-23.
//

#include "Poller.h"


Poller::Poller() {
    pollerFd_ = ::epoll_create1(EPOLL_CLOEXEC);
    eventList_ = std::make_unique<EventList>(16);
    assert(pollerFd_ > 0);
}

Poller::~Poller() {
    if(fcntl(pollerFd_, F_GETFL) == -1) {  // 关闭poller的文件描述符。
        close(pollerFd_);
    }
}

/**
 * 更新channel的关心事件。
 * @param channel
 */
void Poller::updateEpollEvents(int operation, Channel *channel) {
    struct epoll_event event;
    memset(&event, '\0', sizeof(event));
    event.events = channel->events();
    event.data.fd = channel->fd();
    event.data.ptr = channel;
    channel->setRevents(0);  // 重新赋值。
    if(fcntl(channel->fd(), F_GETFL) == -1) {
        LOG << channel->fd() << "文件描述符已经被关闭，不再执行后续任务\n";
    } else {
        assert(::epoll_ctl(pollerFd_, operation, channel->fd(), &event) >= 0);
    }
}

/**
 * 返回pollerFd。
 * @return
 */
int Poller::pollerFd() {
    return pollerFd_;
}

/**
 * 返回EventList
 * @return
 */
EventList &Poller::getEventList() {
    return *eventList_;
}