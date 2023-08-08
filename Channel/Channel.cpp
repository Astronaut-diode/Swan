//
// Created by diode on 23-7-23.
//
#include "Channel.h"
#include <iostream>

void Channel::setReadFunctionCallBack(ReadFunctionCallBack readFunctionCallBack) {
    readFunctionCallBack_ = readFunctionCallBack;
}

void Channel::setWriteFunctionCallBack(WriteFunctionCallBack writeFunctionCallBack) {
    writeFunctionCallBack_ = writeFunctionCallBack;
}

void Channel::setCloseFunctionCallBack(CloseFunctionCallBack closeFunctionCallBack) {
    closeFunctionCallBack_ = closeFunctionCallBack;
}

void Channel::setErrorFunctionCallBack(ErrorFunctionCallBack errorFunctionCallBack) {
    errorFunctionCallBack_ = errorFunctionCallBack;
}

Channel::Channel(int fd) {
    fd_ = fd;
    events_ = 0;
    revents_ = 0;
    useByPoller_ = 0;
}

Channel::~Channel() {
    // 文件描述符的关闭已经在TCP中处理过了，不再处理。
}

void Channel::handleEvent() {  // 当epoll接收到事件的是，分给每个channel自己执行。
    if(useByPoller_ == 0200) {
        readFunctionCallBack_();
    } else
    if (useByPoller_ == 0100) {  // 建立新连接时使用的分支，即acceptorChannel
        readFunctionCallBack_();
    } else if (revents_ & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
        closeFunctionCallBack_();
    } else if (revents_ & EPOLLIN) {
        readFunctionCallBack_();
    }
}

/**
 * 设置channel对读事件感兴趣。
 */
void Channel::enableRead() {
    events_ = events_ | EPOLLIN;
}

void Channel::enableOneShot() {
    events_ = events_ | EPOLLONESHOT;
}

void Channel::enableWrite() {
    events_ = events_ | EPOLLOUT;
}

int Channel::events() {
    return events_;
}

int Channel::fd() {
    return fd_;
}

/**
 * 设置发生了的事件。
 */
void Channel::setRevents(int event) {
    revents_ = event;
}

void Channel::disableRead() {
    events_ = events_ & (~EPOLLIN);
}

void Channel::disableWrite() {
    events_ = events() & (~EPOLLOUT);
}