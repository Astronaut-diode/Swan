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
    SetNonBlocking(connectionFd);  // 一定要设置为非阻塞的，否则使用while接收请求的时候就会出现，第二次接收会被阻塞不动的情况，而不是直接返回-1。
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
    deleteConnectionCallBack_(connectionFd_);
    monitor_ = nullptr;  // 悬空，防止被析构掉。
}


/**
 * connectionChannel受到信息以后的回调事件。
 */
void TcpConnection::handleRead() {

}


void TcpConnection::handleWrite() {

}

void TcpConnection::handleError() {

}

void TcpConnection::SetNonBlocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}