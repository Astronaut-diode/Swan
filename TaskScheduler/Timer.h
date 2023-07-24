//
// Created by diode on 23-7-23.
// 定时器的实现类，每一个定时器包含了触发时间以及触发的内容。

#ifndef SWAN_TIMER_H
#define SWAN_TIMER_H

#include <functional>
#include <cassert>
#include <sys/time.h>

typedef std::function<void()> FunctionCallback;

class Timer {
public:  // 用于写typedef或者静态常量等。

private:  // 变量区域
    FunctionCallback functionCallback_;  // 到时间以后回调的函数。
    timeval expire_;  //  超时的时间。
    double interval_;  // 间隔时间。
    bool repeat_;  // 是否重复执行。
public:

private:  // 函数区域

public:
    Timer(FunctionCallback functionCallback, double interval, bool repeat);

    ~Timer();

    void restart();  // 触发了一次回调函数以后，再次打开定时器。

    void run();  // 运行回调函数。

    bool repeat();

    long long getExpire();

    int getInterval();

    void setExpire(timeval expire);
};

#endif //SWAN_TIMER_H
