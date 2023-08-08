//
// Created by diode on 23-7-24.
//

#include "TcpServer.h"

int TcpServer::kNumber = 0;

TcpServer::TcpServer(InsertToTimeWheelCallBack insertToTimeWheelCallBack,
                     UpdateToTimeWheelCallBack updateToTimeWheelCallBack) {
    threadList_.reserve(kThreadNum);
    monitors_.resize(kThreadNum);
    sem_init(&latch_, 0, 0);
    for (int i = 0; i < kThreadNum; ++i) {  // 创建多个线程。
        char tmp[64]{'\0'};
        strcpy(tmp, kSubThreadName);
        strcat(tmp, std::to_string(i).c_str());  // 构建线程的名字。
        threadList_[i] = new Thread(tmp, std::bind(&TcpServer::launchSubThread, this));  // 创建对应的线程用例。
        threadList_[i]->createThread();  // 创建并启动子线程，让其执行给定的任务。
        sem_wait(&latch_);  // 线程一定要一个个创建。
    }
    distribute_ = 0;  // 轮询使用的参数。
    insertToTimeWheelCallBack_ = insertToTimeWheelCallBack;  // 插入道时间轮中的参数。
    updateToTimeWheelCallBack_ = updateToTimeWheelCallBack;
}

TcpServer::~TcpServer() {
    sem_destroy(&latch_);
}

/**
 * 启动子线程执行任务。
 */
void TcpServer::launchSubThread() {
    Monitor monitor(kSubThreadName, kNumber);  // 创建了Monitor、Epoll、WakeupChannel，并且已经设置好了监听。
    monitors_[kNumber++] = &monitor;
    sem_post(&latch_);
    monitor.loop();
}

/**
 * 使用connectionFd创建一个连接，并且需要将其分到某一个loop上。一定是线程安全的，因为能够接收到连接的线程只有Swan线程。
 * @param connectionFd
 */
void TcpServer::distributeConnection(int connectionFd) {
    int target = (distribute_++) % kThreadNum;
    if (distribute_ >= kThreadNum) {
        distribute_ = 0;
    }
    std::shared_ptr<TcpConnection> conn = std::make_shared<TcpConnection>(connectionFd,
                                                                          monitors_[target],
                                                                          std::bind(&TcpServer::deleteConnection, this,
                                                                                    std::placeholders::_1));  // 创建了TcpConnection以及Channel，并且已经设定了关心读事件。
    sharedConnections_[connectionFd] = conn;
    conn->setInsertTimerWheel(insertToTimeWheelCallBack_);
    conn->setUpdateTimerWheel(updateToTimeWheelCallBack_);
    conn->setGetConnectionSessionsCallBack_(std::bind(&TcpServer::getConnectionSessions, this));
    monitors_[target]->poller_.updateEpollEvents(EPOLL_CTL_ADD, conn->getChannel());  // 让对应的monitor开始监听该连接。
}

void TcpServer::deleteConnection(int connectionFd) {
    sharedConnections_.erase(connectionFd);
}

/**
 * 用于传送ConnectionSession,并进行赋值。
 * @return
 */
std::map<int, TcpConnection> &TcpServer::getConnectionSessions() {
    return connectionSessions_;
}

/**
 * 判断当前机器上的用户id是否有目标存在。存在就插入到对应的Monitor的待执行任务列表中，并执行。
 * @param sourceId 源
 * @param destId 目标
 * @param type 是请求还是信息
 * @return
 */
bool TcpServer::sendInLoop(int sourceId, int destId, int type) {
    if (connectionSessions_.find(destId) != connectionSessions_.end()) {  // 该用户在当前机器上。
        connectionSessions_[destId].monitor_->addSendInLoopCallBack(std::move(
                std::bind(&TcpConnection::send, connectionSessions_[destId], sourceId, destId,
                          type)));  // 本来是要直接发送消息的函数的，但是现在改为插入到monitor的待执行列表中。
        connectionSessions_[destId].monitor_->wakeup();  // 唤醒目标的monitor，不再epoll_wait。
        return true;
    }
    return false;
}