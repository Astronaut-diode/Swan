//
// Created by diode on 23-7-23.
//

#include "TcpConnection.h"


TcpConnection::TcpConnection() {

}

TcpConnection::TcpConnection(int connectionFd, Monitor *monitor, DeleteConnectionCallBack deleteConnectionCallBack)
        : connectionFd_(connectionFd), monitor_(monitor), deleteConnectionCallBack_(deleteConnectionCallBack) {
    connectionChannel_ = new Channel(connectionFd_);
    connectionChannel_->setReadFunctionCallBack(std::bind(&TcpConnection::handleRead, this));
    connectionChannel_->setCloseFunctionCallBack(std::bind(&TcpConnection::handleClose, this));
    connectionChannel_->setWriteFunctionCallBack(std::bind(&TcpConnection::handleWrite, this));
    connectionChannel_->setErrorFunctionCallBack(std::bind(&TcpConnection::handleError, this));
    connectionChannel_->enableRead();
    connectionChannel_->enableOneShot();
    request_ = std::make_unique<Request>(connectionFd_);
    connectionStatement_ = CONNECTION_STATEMENT::HTTP;
}

TcpConnection::~TcpConnection() {

}

Channel *TcpConnection::getChannel() {
    return connectionChannel_;
}

/**
 * 在TcpServer中的动态连接的数组中删除，因为shared_ptr为0了，所以就会析构自己。
 */
void TcpConnection::handleClose() {
    connectionChannel_->disableWrite();
    connectionChannel_->disableRead();
    monitor_->poller_.updateEpollEvents(EPOLL_CTL_DEL, connectionChannel_);
    close(connectionFd_);
    deleteConnectionCallBack_(connectionFd_);
    monitor_ = nullptr;  // 悬空，防止被析构掉。
}


/**
 * connectionChannel受到信息以后的回调事件。
 */
void TcpConnection::handleRead() {
    if(connectionStatement_ == CONNECTION_STATEMENT::WebSocket) {
        request_->receiveWebSocketRequest();
        connectionChannel_->enableRead();
        connectionChannel_->enableOneShot();
        monitor_->poller_.updateEpollEvents(EPOLL_CTL_MOD, connectionChannel_);  // 因为是oneshot所以需要重新监听。
        insertToTimeWheelCallBack_(shared_from_this());
        return;
    }
    if (request_->receiveHTTPRequest()) {  // 如果连接建立成功，那就改变状态。
        connectionStatement_ = CONNECTION_STATEMENT::WebSocket;
        memset(serverKey_, '\0', sizeof(serverKey_));
        strcpy(serverKey_, request_->serverKey_);
    }
    connectionChannel_->enableRead();
    connectionChannel_->enableOneShot();
    monitor_->poller_.updateEpollEvents(EPOLL_CTL_MOD, connectionChannel_);  // 因为是oneshot所以需要重新监听。
    insertToTimeWheelCallBack_(shared_from_this());
}

std::shared_ptr<TcpConnection> TcpConnection::getSharedPtr() {
    return shared_from_this();
}


void TcpConnection::handleWrite() {

}

void TcpConnection::handleError() {

}

void TcpConnection::setInsertTimerWheel(InsertToTimeWheelCallBack insertToTimeWheelCallBack) {
    insertToTimeWheelCallBack_ = insertToTimeWheelCallBack;
}