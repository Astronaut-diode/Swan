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
    request_ = new Request(connectionFd_);
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
    std::shared_ptr<TcpConnection> tmp = shared_from_this();  // 避免执行时被析构。
    connectionChannel_->disableWrite();
    connectionChannel_->disableRead();
    monitor_->poller_.updateEpollEvents(EPOLL_CTL_DEL, connectionChannel_);
    close(connectionFd_);
    deleteConnectionCallBack_(connectionFd_);
    monitor_ = nullptr;  // 悬空，防止被析构掉。
    Redis::get_singleton_()->removeSession(request_->getUserId());
    if (getConnectionSessionsCallBack_().find(userId_) != getConnectionSessionsCallBack_().end()) {
        getConnectionSessionsCallBack_().erase(userId_);  // 删除TcpServer中记录的连接。
    }
}


/**
 * connectionChannel受到信息以后的回调事件。
 */
void TcpConnection::handleRead() {
    std::shared_ptr<TcpConnection> tmp = shared_from_this();  // 避免执行时被析构。
    if (connectionStatement_ == CONNECTION_STATEMENT::WebSocket) {
        if (monitor_) {
            int ret = request_->receiveWebSocketRequest();
            if(ret == -2) {  // 关闭连接。
                handleClose();
                return;
            }
            if (ret != -1) {
                userId_ = ret;
                getConnectionSessionsCallBack_().insert(std::make_pair(ret, *this));
            }
            connectionChannel_->enableRead();
            connectionChannel_->enableOneShot();
            monitor_->poller_.updateEpollEvents(EPOLL_CTL_MOD, connectionChannel_);  // 因为是oneshot所以需要重新监听。
            updateToTimeWheelCallBack_(shared_from_this());
        }
        return;
    }
    bool ret = request_->receiveHTTPRequest();
    if (ret) {  // 如果连接建立成功，那就改变状态。
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

void TcpConnection::setUpdateTimerWheel(UpdateToTimeWheelCallBack updateToTimeWheelCallBack) {
    updateToTimeWheelCallBack_ = updateToTimeWheelCallBack;
}

/**
 * 设置时间轮对应的上下文信息。
 * @param weakTcpConnection
 */
void TcpConnection::setContext(const std::weak_ptr<WeakTcpConnection> &weakTcpConnection) {
    context_ = weakTcpConnection;
}

const std::weak_ptr<WeakTcpConnection> &TcpConnection::getContext() {
    return context_;
}

void
TcpConnection::setGetConnectionSessionsCallBack_(getConnectionSessionsCallBackFunction getConnectionSessionsCallBack) {
    getConnectionSessionsCallBack_ = getConnectionSessionsCallBack;
}

bool TcpConnection::send(int sourceId, int destId, int type) {
    // 请求类型是{"friendMessage", "friendRequest", "groupMessage", "groupRequest", "friendList", "groupList"}
    if (type == 0) {  // 是好友信息，告知目标有来自sourceId的好友信息。
        if (request_->chatId_ == sourceId && !request_->isGroup_) {  // 直接发送新消息，还要判断是否是群聊状态。
            request_->ForceSendMessage(sourceId, destId);
            return true;
        } else {
            request_->ForceUpdateSendAllFriends();  // 强制发送好友名单。
            return true;
        }
    } else if (type == 1) {  // 是添加好友的请求，告知destId，来自sourceId的好友请求。
        request_->pushAddFriendRequestMessage(sourceId, destId);
        return true;
    } else if (type == 2) {  // 是群组信息，告知目标群组的所有用户有来自群组的新信息。
        if (request_->chatId_ == sourceId && request_->isGroup_) {  // 直接发送新消息，还要判断是否是群聊状态。
            request_->ForceSendGroupMessage(sourceId);
            return true;
        } else {
            request_->ForceUpdateSendAllGroups();  // 强制发送群组名单。
            return true;
        }
    } else if (type == 3) {  // 是添加群的请求，告知destId，来自sourceId的群添加请求。
        request_->pushAddGroupRequestMessage(sourceId, destId);
        return true;
    } else if (type == 4) {  // 推送好友名单。
        request_->sendAllFriends();  // 发送好友名单。
        return true;
    } else if (type == 5) {
        request_->sendAllGroups();  // 发送群组名单。
        return true;
    }
    return false;
}