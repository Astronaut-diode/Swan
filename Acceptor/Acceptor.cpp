//
// Created by diode on 23-7-24.
//

#include "Acceptor.h"

int Acceptor::createListenSocket() {  // 建立一个监听使用的文件描述符。
    int sockfd = ::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    return sockfd;
}

void Acceptor::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(acceptorFd_, SOL_SOCKET, SO_REUSEADDR,
                 &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}

void Acceptor::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(acceptorFd_, SOL_SOCKET, SO_REUSEPORT,
                           &optval, static_cast<socklen_t>(sizeof optval));
}

Acceptor::Acceptor(Monitor *monitor, DistributeConnectionCallBack distributeConnectionCallBack) {
    mainMonitor_ = monitor;
    acceptorFd_ = createListenSocket();
    setReuseAddr(true);
    setReusePort(true);
    acceptorAddress_ = Address(kPort);  // 需要监听的地址。
    int ret = ::bind(acceptorFd_, (sockaddr *) &acceptorAddress_.getAddr(),
                     static_cast<socklen_t>(sizeof(struct sockaddr_in)));
    assert(ret >= 0);
    ret = ::listen(acceptorFd_, SOMAXCONN);
    assert(ret >= 0);
    acceptorChannel_ = new Channel(acceptorFd_);
    acceptorChannel_->setReadFunctionCallBack(std::bind(&Acceptor::establishConnection, this));
    acceptorChannel_->enableRead();
    mainMonitor_->poller_.updateEpollEvents(EPOLL_CTL_ADD, acceptorChannel_);
    distributeConnectionCallBack_ = distributeConnectionCallBack;
}

Acceptor::~Acceptor() {

}

/**
 * 给出对应的acceptorChannel。
 * @return
 */
Channel *Acceptor::getAcceptorChannel() {
    return acceptorChannel_;
}

/**
 * 当acceptorChannel收到了信号以后，准备建立连接。
 */
void Acceptor::establishConnection() {
    sockaddr_in client_address;
    int client_address_size = sizeof(client_address);
    int connectionFd = accept(acceptorFd_, (sockaddr *) &client_address, (socklen_t *) &client_address_size);
    distributeConnectionCallBack_(connectionFd);  // 预备建立连接，并将连接分给别的epoll。
}