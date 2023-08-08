//
// Created by diode on 23-7-24.
//

#include "Monitor.h"


__thread Monitor *threadMonitor;  // 每个线程都会有一个独立的threadMonitor，进行绑定。

Monitor::Monitor() : wakeupFd_(createEventfd()),
                     wakeupChannel_(new Channel(wakeupFd_)), poller_() {  // 主线程的Monitor函数。
    Utils::setNonBlocking(wakeupFd_);  // 设置文件描述符非阻塞。
    wakeupChannel_->setReadFunctionCallBack(std::bind(&Monitor::handleWakeRead, this));
    wakeupChannel_->enableRead();
    poller_.updateEpollEvents(EPOLL_CTL_ADD, wakeupChannel_);
    strcpy(threadName_, "Swan");
    assert(!threadMonitor);  // 创建之前肯定不能存在。
    threadMonitor = this;
}

Monitor::Monitor(const char *threadName, int num) : wakeupFd_(createEventfd()),
                                                    wakeupChannel_(new Channel(wakeupFd_)),
                                                    poller_() {
    wakeupChannel_->setReadFunctionCallBack(std::bind(&Monitor::handleWakeRead, this));
    wakeupChannel_->enableRead();
    poller_.updateEpollEvents(EPOLL_CTL_ADD, wakeupChannel_);
    strcpy(threadName_, threadName);
    if (num != -1) {
        strcat(threadName_, std::to_string(num).c_str());
    }
    assert(!threadMonitor);  // 创建之前肯定不能存在。
    threadMonitor = this;
}

/**
 * 创建新的wakeup的文件描述符。
 * @return
 */
int Monitor::createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    assert(evtfd >= 0);
    return evtfd;
}

Monitor::~Monitor() {

}

/**
 * 子线程的wakeupChannel被唤醒的时候调用的回调函数。
 */
void Monitor::handleWakeRead() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    assert(n == sizeof one);
}

/**
 * 唤醒epoll_wait。
 */
void Monitor::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    assert(n == sizeof one);
}

/**
 * 进行loop的死循环操作。
 */
void Monitor::loop() {
    while (true) {
        int num = ::epoll_wait(this->poller_.pollerFd(), &*(this->poller_.getEventList().begin()),
                               static_cast<int>(this->poller_.getEventList().size()), -1);
        if (num == -1 && errno != EINTR) {
            return;
        }
        std::vector<Channel *> activateChannels;
        for (int i = 0; i < num; ++i) {
            activateChannels.push_back(static_cast<Channel *>(this->poller_.getEventList()[i].data.ptr));
            activateChannels.back()->setRevents(this->poller_.getEventList()[i].events);
        }
        for (int i = 0; i < activateChannels.size(); ++i) {
            activateChannels[i]->handleEvent();
        }
        for (int i = 0; i < sendInLoopCallBacks_.size(); ++i) {
            sendInLoopCallBacks_[i](-1, -1, -1);  // 执行当前monitor待执行的内容。
        }
        sendInLoopCallBacks_.clear();
    }
}

void Monitor::addSendInLoopCallBack(SendCallBack sendCallBack) {
    sendInLoopCallBacks_.emplace_back(sendCallBack);
}