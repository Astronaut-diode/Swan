#include "Logger/AsyncLog.h"
#include "TaskScheduler/TaskScheduler.h"
#include "TcpServer/TcpServer.h"
#include "Acceptor/Acceptor.h"

AsyncLog asyncLog(true);  // 定义为全局的变量,方便LogStream析构的时候，将内容写入到异步线程中。

int main() {
    TaskScheduler taskScheduler;
    TcpServer tcpServer(std::bind(&TaskScheduler::insertToConnections, &taskScheduler, std::placeholders::_1), std::bind(&TaskScheduler::updateToConnections, &taskScheduler, std::placeholders::_1));
    Monitor monitor;  // 创建主线程的monitor，并开始loop。
    Acceptor acceptor(&monitor, std::bind(&TcpServer::distributeConnection, &tcpServer, std::placeholders::_1));
    monitor.loop();
    return 0;
}