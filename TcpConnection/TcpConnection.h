//
// Created by diode on 23-7-23.
// 连接类，暂时为空，仅仅当作占位符。

#ifndef SWAN_TCPCONNECTION_H
#define SWAN_TCPCONNECTION_H

#include <iostream>
#include <sys/epoll.h>
#include <functional>
#include "../Channel/Channel.h"
#include "../Logger/LogStream.h"
#include "../Monitor/Monitor.h"
//#include "../TaskScheduler/TaskScheduler.h"
#include "Request.h"

typedef std::function<void(int)> DeleteConnectionCallBack;
class TcpConnection;
struct WeakTcpConnection;

typedef std::function<void(const std::shared_ptr<TcpConnection> &tcpConnection)> InsertToTimeWheelCallBack;
typedef std::function<void(const std::shared_ptr<TcpConnection> &tcpConnection)> UpdateToTimeWheelCallBack;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:  // 用于写typedef或者静态常量等。
    enum CONNECTION_STATEMENT {  // 当前的连接处于什么状态。
        HTTP = 0,
        WebSocket
    };
private:  // 变量区域
    int connectionFd_;  // 使用的文件描述符。
    Channel *connectionChannel_;  // 对应的channel。
    Monitor *monitor_;
    DeleteConnectionCallBack deleteConnectionCallBack_;  // 删除Tcpserver中connections的回调函数。
    Request *request_;  // 该连接对应的request。
    CONNECTION_STATEMENT connectionStatement_;  // 当前连接的状态。
    InsertToTimeWheelCallBack insertToTimeWheelCallBack_;
    InsertToTimeWheelCallBack updateToTimeWheelCallBack_;
    char serverKey_[20];

public:
    std::weak_ptr<WeakTcpConnection> context_;  // 用于响应时间轮的上下文环境。

private:  // 函数区域
    std::shared_ptr<TcpConnection> getSharedPtr();

public:
    TcpConnection();

    TcpConnection(int connectionFd, Monitor *monitor, DeleteConnectionCallBack deleteConnectionCallBack);

    ~TcpConnection();

    Channel *getChannel();

    void handleClose();

    /**
     * connectionChannel受到信息以后的回调事件。
     */
    void handleRead();

    void handleWrite();

    void handleError();

    void setInsertTimerWheel(InsertToTimeWheelCallBack insertToTimeWheelCallBack);

    void setUpdateTimerWheel(UpdateToTimeWheelCallBack updateToTimeWheelCallBack);

    void setContext(const std::weak_ptr<WeakTcpConnection> &weakTcpConnection);  // 设置时间轮对应的信息

    const std::weak_ptr<WeakTcpConnection> &getContext();  // 获取对应的时间轮上下文信息。
};

#endif //SWAN_TCPCONNECTION_H