//
// Created by diode on 23-7-23.
//

#include "TaskScheduler.h"


TaskScheduler::TaskScheduler() {
    timerQueue_ = std::make_unique<TimerQueue>();
    timerQueue_->addTimer(std::bind(&TaskScheduler::rotateTimeWheel, this), 1, true);  // 开始循环不断的触发对于时间轮的监听事务，转动时间轮。
    timeWheel_ = std::make_unique<connectionList>();
    thread_ = std::make_unique<Thread>(kTimerThreadName,
                                       std::bind(&TaskScheduler::launchTimeChannel, this));  // 创建对应的线程用例。
    thread_->createThread();  // 创建并启动子线程，让其执行给定的任务。
}

TaskScheduler::~TaskScheduler() {

}

/**
 * 显示当前的时间轮情况。
 */
void TaskScheduler::dump() {
    std::stringstream buffer;
    buffer << "时间轮内容:\n";
    for (int i = 0; i < timeWheel_->size(); ++i) {
        buffer << "[" << i << "]===>";
        for (std::unordered_set<sharedWeakTcpConnection>::iterator ite = (*timeWheel_)[i].begin();
             ite != (*timeWheel_)[i].end(); ++ite) {
            buffer << static_cast<void *>((*ite).get()->weakPtr_.lock().get()) << ":" << (*ite).use_count() << " ";
        }
        buffer << "\n";
    }
    LOG << buffer.str().c_str();
}

/**
 * 旋转时间轮。
 */
void TaskScheduler::rotateTimeWheel() {
    dump();  // 因为每一次定时dump的时候，都会移动一个Bucket,所以是看不见插入的内容在结尾的时候的，需要在这里手动看一编。
    timeWheel_->push_back(Bucket());  // 排挤出第一个元素。
}

/**
 * 启动Time线程，并在其中启动一个专属的epoll监听事件以及timeChannel。
 */
void TaskScheduler::launchTimeChannel() {
    while (true) {
        int num = ::epoll_wait(timerQueue_->pollerFd(), &*(timerQueue_->getPoller().getEventList().begin()),
                               static_cast<int>(timerQueue_->getPoller().getEventList().size()), -1);
        if (num == -1 && errno != EINTR) {
            return;
        }
        std::vector<Channel *> activateChannels;
        for (int i = 0; i < num; ++i) {
            activateChannels.push_back(static_cast<Channel *>(timerQueue_->getPoller().getEventList()[i].data.ptr));
            activateChannels.back()->setRevents(timerQueue_->getPoller().getEventList()[i].events);
        }
        for (int i = 0; i < activateChannels.size(); ++i) {
            activateChannels[i]->handleEvent();
        }
    }
}

/**
 * 将新的连接插入到时间轮中。
 * @param tcpConnection
 */
void TaskScheduler::insertToConnections(const std::shared_ptr<TcpConnection> &tcpConnection) {
    // 注意，这个TcpConnection肯定是已经被记录到share_ptr了，为了让关闭连接的时候可以erase直接析构对象，我们这里不能再次设计为share_ptr,只能够使用weak_ptr，仅仅用于记录这个对象是否存在，如果存在，析构的时候调用stop函数就行。
    std::shared_ptr<WeakTcpConnection> weakTcpConnection = std::make_shared<WeakTcpConnection>(tcpConnection);
    tcpConnection->setContext(weakTcpConnection);
    (*timeWheel_).data_.back().insert(weakTcpConnection);
}

/**
 * 发送信息的时候，更新时间轮。
 * @param tcpConnection
 */
void TaskScheduler::updateToConnections(const std::shared_ptr<TcpConnection> &tcpConnection) {
    (*timeWheel_).data_.back().insert(tcpConnection->context_.lock());
}