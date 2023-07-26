//
// Created by diode on 23-7-24.
//

#ifndef SWAN_ACCEPTOR_H
#define SWAN_ACCEPTOR_H

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <functional>
#include "../Channel/Channel.h"
#include "../TcpConnection/Address.h"
#include "../Monitor/Monitor.h"
#include "../Logger/LogStream.h"

typedef Monitor MainMonitor;  // 在当前文件中设置别名。
typedef std::function<void(int)> DistributeConnectionCallBack;  // 设置分发连接的回购函数。

class Acceptor {
public:  // 用于写typedef或者静态常量等。
    static const int kPort = 8081;
private:  // 变量区域
    int acceptorFd_;  // 用于唤醒使用的文件描述符。
    Channel *acceptorChannel_;  // 该文件描述符相关联的channel。
    Address acceptorAddress_;  // 监听使用的网络地址。
    MainMonitor *mainMonitor_;
    DistributeConnectionCallBack distributeConnectionCallBack_;
public:

private:  // 函数区域
    int createListenSocket();

    void setReuseAddr(bool on);

    void setReusePort(bool on);

    void setKeepAlive(bool on);

    void SetNonBlocking(int fd);
public:
    Acceptor(Monitor *monitor, DistributeConnectionCallBack distributeConnectionCallBack);

    ~Acceptor();

    Channel *getAcceptorChannel();

    void establishConnection();
};

#endif //SWAN_ACCEPTOR_H
