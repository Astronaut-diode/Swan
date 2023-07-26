//
// Created by diode on 23-7-23.
// 连接类，暂时为空，仅仅当作占位符。

#ifndef SWAN_TCPCONNECTION_H
#define SWAN_TCPCONNECTION_H

#include <iostream>
#include <sys/epoll.h>
#include <functional>
#include <fcntl.h>
#include "../Channel/Channel.h"
#include "../Logger/LogStream.h"
#include "../Monitor/Monitor.h"

typedef std::function<void(int)> DeleteConnectionCallBack;

class TcpConnection {
public:  // 用于写typedef或者静态常量等。

private:  // 变量区域
    int connectionFd_;  // 使用的文件描述符。
    Channel *connectionChannel_;  // 对应的channel。
    Monitor *monitor_;
    DeleteConnectionCallBack deleteConnectionCallBack_;  // 删除Tcpserver中connections的回调函数。
public:

private:  // 函数区域
    static void SetNonBlocking(int fd);
public:
    TcpConnection();

    TcpConnection(int connectionFd, Monitor *monitor, DeleteConnectionCallBack deleteConnectionCallBack);

    ~TcpConnection();

    Channel *getChannel();

    void handleClose() ;

    /**
     * connectionChannel受到信息以后的回调事件。
     */
    void handleRead();

    void handleWrite();

    void handleError();
};

#endif //SWAN_TCPCONNECTION_H
