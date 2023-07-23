//
// Created by diode on 23-7-22.
// 线程类，可以用于创建一个线程，并使用该线程执行某个固定的回调函数。

#ifndef SWAN_THREAD_H
#define SWAN_THREAD_H

#include <pthread.h>
#include <semaphore.h>
#include <cstring>
#include <functional>
#include <cassert>
#include <sys/prctl.h>

class Thread {
public:  // 用于写typedef或者静态常量等。
    typedef std::function<void()> ThreadCallBackFunction;
    static const int kThreadNameLength = 256;  // 线程的名字最长长度。
private:  // 变量区域
    bool running_;  // 线程是否已经开始运行。
    sem_t waitMainThreadLatch_;  // 等待主线程建立成功的信号量。
    pthread_t threadId_;  // 对应的线程的id。
    char threadName_[kThreadNameLength];  // 线程的名字。
    ThreadCallBackFunction threadCallBackFunction_;  // 线程启动以后执行的回调函数。

public:

private:  // 函数区域

public:
    Thread(const char *threadName, ThreadCallBackFunction threadCallBackFunction);

    ~Thread();

    void createThread();  // 创建对应的线程。

    void startThread();  // 启动线程，执行给定的回调函数。
};

#endif //SWAN_THREAD_H
