//
// Created by diode on 23-7-23.
//

#include "Timer.h"

Timer::Timer(FunctionCallback functionCallback, double interval, bool repeat) {
    functionCallback_ = functionCallback;
    interval_  = interval;
    repeat_ = repeat;
    struct timeval now;
    ::gettimeofday(&now, nullptr);
    expire_.tv_sec = now.tv_sec + interval;  // 设置下一次超时响应的时间。
}

Timer::~Timer() {

}

void Timer::restart() {
    if(repeat_) {
        struct timeval now;
        ::gettimeofday(&now, nullptr);
        expire_.tv_sec = now.tv_sec + interval_;  // 设置下一次超时响应的时间。
    }
}

/**
 * 执行定时器的超时回调函数。
 */
void Timer::run() {
    functionCallback_();
}


bool Timer::repeat() {
    return repeat_;
}

long long Timer::getExpire() {
    return expire_.tv_sec;
}

int Timer::getInterval() {
    return interval_;
}

void Timer::setExpire(timeval expire) {
    expire_ = expire;
}