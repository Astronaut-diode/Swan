//
// Created by diode on 23-7-24.
//

#ifndef SWAN_TCPSERVER_H
#define SWAN_TCPSERVER_H

#include <vector>
#include <memory>
#include <pthread.h>
#include <functional>
#include <map>
#include "../Monitor/Monitor.h"
#include "../Logger/LogStream.h"
#include "../Thread/Thread.h"
#include "../TcpConnection/TcpConnection.h"

typedef std::vector<std::unique_ptr<Thread>> ThreadList;
typedef std::function<void(const std::shared_ptr<TcpConnection> &tcpConnection)> InsertToTimeWheelCallBack;
typedef std::function<void(const std::shared_ptr<TcpConnection> &tcpConnection)> UpdateToTimeWheelCallBack;

class TcpServer {
public:  // 用于写typedef或者静态常量等。
    static const int kThreadNum = 8;  // 共计开启几个子线程用于处理任务。
    const char *kSubThreadName = "subThread-";
    static int kNumber;
private:  // 变量区域
    std::vector<Thread*> threadList_;  // 记录所有的子线程
    std::vector<Monitor *> monitors_;  // 记录所有的monitor。
    sem_t latch_;
    std::map<int, std::shared_ptr<TcpConnection>> sharedConnections_;  // 所有的连接，并且使用了shared指针。;
    std::map<int, TcpConnection> connectionSessions_;  // 记录已经连接的用户使用的session是多少。
    int distribute_;  // 用于轮询查询的数字。
    InsertToTimeWheelCallBack insertToTimeWheelCallBack_;
    UpdateToTimeWheelCallBack updateToTimeWheelCallBack_;
public:

private:  // 函数区域

public:
    TcpServer(InsertToTimeWheelCallBack insertToTimeWheelCallBack, UpdateToTimeWheelCallBack updateToTimeWheelCallBack);

    ~TcpServer();

    void launchSubThread();  // 启动子线程执行任务。

    void distributeConnection(int connectionFd);  // 使用connectionFd创建一个连接，并且需要将其分到某一个loop上。

    void deleteConnection(int connectionFd);  // 删除connection。

    std::map<int, TcpConnection> &getConnectionSessions();

    bool sendInLoop(int sourceId, int destId, int type);
};

#endif //SWAN_TCPSERVER_H
