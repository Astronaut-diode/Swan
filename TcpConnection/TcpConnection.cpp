//
// Created by diode on 23-7-23.
//

#include "TcpConnection.h"


TcpConnection::TcpConnection() {
    std::cout << "TcpConnection()" << std::endl;
}

TcpConnection::TcpConnection(int connectionFd, Monitor *monitor, DeleteConnectionCallBack deleteConnectionCallBack)
        : connectionFd_(connectionFd), monitor_(monitor), deleteConnectionCallBack_(deleteConnectionCallBack) {
    std::cout << "TcpConnection(int connectionFd)" << std::endl;
    connectionChannel_ = new Channel(connectionFd_);
    connectionChannel_->setReadFunctionCallBack(std::bind(&TcpConnection::handleRead, this));
    connectionChannel_->setCloseFunctionCallBack(std::bind(&TcpConnection::handleClose, this));
    connectionChannel_->setWriteFunctionCallBack(std::bind(&TcpConnection::handleWrite, this));
    connectionChannel_->setErrorFunctionCallBack(std::bind(&TcpConnection::handleError, this));
    connectionChannel_->enableRead();
}

TcpConnection::~TcpConnection() {
    std::cout << "~TcpConnection()" << std::endl;
}

Channel *TcpConnection::getChannel() {
    return connectionChannel_;
}

/**
 * 在TcpServer中的动态连接的数组中删除，因为shared_ptr为0了，所以就会析构自己。
 */
void TcpConnection::handleClose() {
    std::cout << "准备关停当前对象，并析构" << std::endl;
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
    char buffer[1024]{'\0'};
    read(connectionFd_, buffer, 1024);
    LOG << "收到了" << connectionFd_ << "的信息,内容是" << buffer << "\n";
}


void TcpConnection::handleWrite() {

}

void TcpConnection::handleError() {

}