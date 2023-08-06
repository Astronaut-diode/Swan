#include "Logger/AsyncLog.h"
#include "TaskScheduler/TaskScheduler.h"
#include "TcpServer/TcpServer.h"
#include "Acceptor/Acceptor.h"
#include "Config/Config.h"
#include "MysqlConnectionPool/MysqlConnectionPool.h"
#include "Redis/Redis.h"

AsyncLog asyncLog(true);  // 定义为全局的变量,方便LogStream析构的时候，将内容写入到异步线程中。

int main() {
    Config::get_singleton_();
    TaskScheduler taskScheduler;
    TcpServer tcpServer(std::bind(&TaskScheduler::insertToConnections, &taskScheduler, std::placeholders::_1), std::bind(&TaskScheduler::updateToConnections, &taskScheduler, std::placeholders::_1));
    Monitor monitor;  // 创建主线程的monitor，并开始loop。
    MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_();  // 创建mysql和redis连接池的单例对象。
    Redis::get_singleton_();
    Acceptor acceptor(&monitor,
                      std::bind(&TcpServer::distributeConnection, &tcpServer, std::placeholders::_1),
                      std::bind(&TcpServer::sendInLoop, &tcpServer, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    monitor.loop();
    return 0;
}