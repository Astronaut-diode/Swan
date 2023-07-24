//
// Created by diode on 23-7-23.
//

#include <iostream>
#include "Poller.h"

Poller::Poller() {
    pollerFd_ = ::epoll_create1(EPOLL_CLOEXEC);
    eventList_ = std::make_unique<EventList>(16);
    assert(pollerFd_ > 0);
}

Poller::~Poller() {

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
    assert(::epoll_ctl(pollerFd_, operation, channel->fd(), &event) >= 0);
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