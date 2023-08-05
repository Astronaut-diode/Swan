//
// Created by diode on 23-7-23.
// 时间任务的管理模块，其中包含两个主要的子模块，时间轮以及定时器队列。

#ifndef SWAN_TASKSCHEDULER_H
#define SWAN_TASKSCHEDULER_H

#include <memory>
#include <unordered_set>
#include <boost/circular_buffer.hpp>
#include "CircleBuffer.h"
#include "TimerQueue.h"
#include "../Logger/LogStream.h"
#include "../Thread/Thread.h"
#include "../TcpConnection/TcpConnection.h"

class TcpConnection;

struct WeakTcpConnection {
    std::weak_ptr<TcpConnection> weakPtr_;

    explicit WeakTcpConnection(const std::shared_ptr<TcpConnection> &conn):weakPtr_(conn) {

    }

    ~WeakTcpConnection() {  // 进行析构，将weakPtr的结果转换为shared_ptr，并调用关闭连接的操作。
        std::shared_ptr<TcpConnection> p = weakPtr_.lock();
        if(p) {  // 因为是weak指针，所以原始内容被释放了的话，这里结果就是空，否则就不会是空。
            (*p).handleClose();
        }
    }
};

class TaskScheduler {
public:  // 用于写typedef或者静态常量等。
    static const int kLen = 1 * 60;  // 代表长时间不操作被踢出时间轮的值。
    const char *kTimerThreadName = "Timer\0";  // 开启定时器异步功能的时候，定时器线程的名字。
    typedef std::shared_ptr<WeakTcpConnection> sharedWeakTcpConnection;
    typedef std::unordered_set<sharedWeakTcpConnection> Bucket;
    typedef CircleBuffer<kLen, Bucket> connectionList;
private:  // 变量区域
    std::unique_ptr<TimerQueue> timerQueue_;  // 定时器列表。
    std::unique_ptr<connectionList> timeWheel_;  // 时间轮，每一个bucket中保存的是unordered_set<shared_ptr<WeakTcpConnection>>;
    std::unique_ptr<Thread> thread_;  // 预备开启定时器线程。
public:

private:  // 函数区域

public:
    TaskScheduler();

    ~TaskScheduler();

    void dump();  // 显示当前的时间轮情况。

    void rotateTimeWheel();  // 旋转时间轮。

    void launchTimeChannel();  // 启动Time线程，并在其中启动一个专属的epoll监听事件以及timeChannel。

    void insertToConnections(const std::shared_ptr<TcpConnection> &tcpConnection);  // 将新的连接插入到时间轮中。

    void updateToConnections(const std::shared_ptr<TcpConnection> &tcpConnection);  // 发送信息的时候，更新时间轮。
};

#endif //SWAN_TASKSCHEDULER_H
