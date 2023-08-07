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
}

void Acceptor::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(acceptorFd_, SOL_SOCKET, SO_REUSEPORT,
                           &optval, static_cast<socklen_t>(sizeof optval));
}

Acceptor::Acceptor(Monitor *monitor, DistributeConnectionCallBack distributeConnectionCallBack,
                   SendInLoopCallBack sendInLoopCallBack) {
    mainMonitor_ = monitor;
    acceptorFd_ = createListenSocket();
    Utils::setNonBlocking(acceptorFd_);  // 设置非阻塞。
    setReuseAddr(true);
    setReusePort(true);
    acceptorAddress_ = Address(kPort);  // 需要监听的地址。
    int ret = ::bind(acceptorFd_, (sockaddr *) &acceptorAddress_.getAddr(),
                     static_cast<socklen_t>(sizeof(struct sockaddr_in)));
    assert(ret >= 0);
    ret = ::listen(acceptorFd_, SOMAXCONN);
    assert(ret >= 0);
    acceptorChannel_ = new Channel(acceptorFd_);
    acceptorChannel_->useByPoller_ = 0100;  // 代表是acceptor的文件描述符。
    acceptorChannel_->setReadFunctionCallBack(std::bind(&Acceptor::establishConnection, this));
    acceptorChannel_->enableRead();
    mainMonitor_->poller_.updateEpollEvents(EPOLL_CTL_ADD, acceptorChannel_);
    distributeConnectionCallBack_ = distributeConnectionCallBack;
    sendInLoopCallBack_ = sendInLoopCallBack;
    // 顺带订阅redis的四个频道
    std::string names[] = {"friendMessage", "friendRequest", "groupMessage", "groupRequest", "friendList", "groupList"};
    for (int i = 0; i < 6; ++i) {  // 开始订阅六个频道。
        redisContexts_[i] = nullptr;
        assert(Redis::get_singleton_()->GetConnection(&redisContexts_[i]));
        // 订阅频道
        redisReply *redisReply1 = (redisReply *) redisCommand(redisContexts_[i], "SUBSCRIBE %s", names[i].c_str());
        freeReplyObject(redisReply1);
        int subscribeFd = redisContexts_[i]->fd;
        Utils::setNonBlocking(subscribeFd);
        subscribeChannel_[i] = new Channel(subscribeFd);
        subscribeChannel_[i]->useByPoller_ = 0200;  // 代表是acceptor的文件描述符。
        subscribeChannel_[i]->setReadFunctionCallBack(
                std::bind(&Acceptor::subscribeChannelReadCallback, this, i));  // i用于代表这个频道是第几个频道，处理不同的内容。
        subscribeChannel_[i]->enableRead();
        mainMonitor_->poller_.updateEpollEvents(EPOLL_CTL_ADD, subscribeChannel_[i]);
    }
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
    Utils::setNonBlocking(connectionFd);
    distributeConnectionCallBack_(connectionFd);  // 预备建立连接，并将连接分给别的epoll。
}

/**
 * 当redis的订阅频道收到信息以后，使用这个函数。
 */
void Acceptor::subscribeChannelReadCallback(int i) {
    // 有数据到达，接收消息
    redisReply *reply;
    redisGetReply(redisContexts_[i], (void **) &reply);
    if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3) {
        // 接收到订阅消息
        if (reply->element[0]->type == REDIS_REPLY_STRING &&
            reply->element[1]->type == REDIS_REPLY_STRING &&
            reply->element[2]->type == REDIS_REPLY_STRING) {
            char sourceId[10]{'\0'}, destId[10]{'\0'};
            ssize_t index = strchr(reply->element[2]->str, ':') - reply->element[2]->str;
            strncpy(sourceId, reply->element[2]->str, index);
            strcpy(destId, strchr(reply->element[2]->str, ':') + 1);
            sendInLoopCallBack_(atoi(sourceId), atoi(destId), i);  // 请求类型，并填入sourceId还有destId。
        }
    }
    freeReplyObject(reply);
}