//
// Created by diode on 23-7-23.
//

#include "TimerQueue.h"

/**
 * 创建时间的文件描述符
 * @return
 */
int TimerQueue::createTimerFd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);
    assert(timerfd > 0);
    return timerfd;
}

/**
 * 向定时器列表中增加一个定时器,并返回超时的时间。
 * @param cb
 * @param interval
 * @param repeat
 * @return
 */
timeval TimerQueue::addTimer(FunctionCallback cb, double interval, bool repeat) {
    struct timeval now;  // 找出当前的时间。
    ::gettimeofday(&now, nullptr);
    timeval expire;
    expire.tv_sec = now.tv_sec + interval;  // 设置超时时刻。
    Timer *timer = new Timer(cb, interval, repeat);  // 设置需要插入的定时器。
    bool moreEarly = insert(timer, expire);
    if (moreEarly) {  // 重新设置超时时间。
        resetTimerfd(expire);
    }
    return expire;
}

/**
 * 重新设置监听下一次提醒的时间。
 * @param expire
 */
void TimerQueue::resetTimerfd(timeval expire) {
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof newValue);
    memset(&oldValue, 0, sizeof oldValue);
    struct timeval now;
    gettimeofday(&now, nullptr);
    newValue.it_value.tv_sec = expire.tv_sec - now.tv_sec;
    int ret = ::timerfd_settime(timerFd_, 0, &newValue, &oldValue);
    assert(ret == 0);
}

/**
 * 将timer插入到timers_中。并判断是否会刷新最小的超时记录。
 * @param timer
 * @return
 */
bool TimerQueue::insert(Timer *timer, timeval expire) {
    bool early = false;
    TimerList::iterator ite = timers_.begin();
    if (ite == timers_.end() || expire.tv_sec < ite->first) {  // 如果当前的内容为空，或者超时时间小于最小的那一个，那就说明需要设置early。
        early = true;
    }
    timers_.insert(std::make_pair(expire.tv_sec, timer));
    return early;
}

TimerQueue::TimerQueue() : timerFd_(createTimerFd()),
                           timeChannel_(new Channel(timerFd_)),
                           poller_() {
    timeChannel_->setReadFunctionCallBack(std::bind(&TimerQueue::processRead, this));
    timeChannel_->enableRead();  // 设置为关心读事件，并且直接加入到epoll的监听中。
    poller_.updateEpollEvents(EPOLL_CTL_ADD, timeChannel_);
}

TimerQueue::~TimerQueue() {
    close(timerFd_);
}

int TimerQueue::pollerFd() {
    return poller_.pollerFd();
}

Poller& TimerQueue::getPoller() {
    return poller_;
}

/**
 * 处理channel返回的读事件。
 */
void TimerQueue::processRead() {
    // 将内容读取掉，没啥用了。
    uint64_t howmany;
    ssize_t n = ::read(timerFd_, &howmany, sizeof howmany);
    // 准备找出所有的过期事件，然后执行每一个过期事件的回调事件。并将某些定时器重新插入回去。
    std::vector<TimerEntry> expired = getExpired();
    for (const TimerEntry &it: expired) {
        it.second->run();
    }
    reset(expired);
}


/**
 * 找出所有的过期定时器。
 * @param time
 * @return
 */
std::vector<TimerEntry> TimerQueue::getExpired() {
    std::vector<TimerEntry> expires;
    struct timeval now;
    gettimeofday(&now, nullptr);
    TimerEntry sentry(now.tv_sec, reinterpret_cast<Timer *>(UINTPTR_MAX));
    TimerList::iterator end = timers_.lower_bound(sentry);
    std::copy(timers_.begin(), end, back_inserter(expires));
    timers_.erase(timers_.begin(), end);

    return expires;
}

/**
 * 检查超时的内容是否需要重新插入回去。
 * @param expired
 */
void TimerQueue::reset(std::vector<TimerEntry> expired) {
    for (const TimerEntry &it: expired) {
        if (it.second->repeat()) {
            struct timeval now;
            gettimeofday(&now, nullptr);
            struct timeval expire;
            expire.tv_sec = now.tv_sec + it.second->getInterval();
            it.second->setExpire(expire);
            insert(it.second, expire);
        } else {
            delete it.second;
        }

        if (!timers_.empty()) {
            struct timeval target;
            target.tv_sec = timers_.begin()->second->getExpire();
            resetTimerfd(target);
        }
    }
}