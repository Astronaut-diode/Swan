//
// Created by diode on 23-7-22.
// 线程类，可以用于创建一个线程，并使用该线程执行某个固定的回调函数。

#include "Thread.h"


Thread::Thread(const char *threadName, ThreadCallBackFunction threadCallBackFunction) {
    running_ = false;
    sem_init(&waitMainThreadLatch_, 0, 0);  // 第二个参数默认是0，第三个参数才是起始值。
    threadId_ = 0;  // 线程还没有创建，设置对应的线程id为0。
    memset(threadName_, '\0', kThreadNameLength);
    memcpy(threadName_, threadName, strlen(threadName));  // 预先设置线程名字。
    threadCallBackFunction_ = threadCallBackFunction;
}

Thread::~Thread() {
    sem_destroy(&waitMainThreadLatch_);
}

void *start_routine(void *args) {
    Thread *t = (Thread *)args;
    t->startThread();
}

/**
 * 创建一个线程。
 */
void Thread::createThread() {
    assert(!running_);
    running_ = true;
    assert(pthread_create(&threadId_, nullptr, start_routine, this) == 0);
    sem_wait(&waitMainThreadLatch_);  // 等待子线程创建完全成功，主线程才能继续执行。
}

/**
 * 启动线程，执行给定的回调函数。
 */
void Thread::startThread() {
    ::prctl(PR_SET_NAME, threadName_);  // 设置线程的名字。
    sem_post(&waitMainThreadLatch_);  // 解开latch，让主线程继续执行。
    threadCallBackFunction_();  // 让子线程去执行给定的回调函数。
}