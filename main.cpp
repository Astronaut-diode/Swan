#include <vector>
#include "Logger/AsyncLog.h"
#include "TaskScheduler/TaskScheduler.h"

AsyncLog asyncLog(true);  // 定义为全局的变量,方便LogStream析构的时候，将内容写入到异步线程中。

int main() {
    TaskScheduler taskScheduler;
    // todo:将连接记录到tcpserver中。
    std::vector<std::shared_ptr<TcpConnection>> connections_;

    while (true) {
        {
            std::shared_ptr<TcpConnection> con = std::make_shared<TcpConnection>();
            std::cout << "当前的use_count:" << con.use_count() << std::endl;
            std::cout << "地址是" << con.get() << std::endl;
            connections_.push_back(con);
            std::cout << "当前的use_count:" << con.use_count() << std::endl;
            std::cout << "地址是" << connections_[0].get() << std::endl;
            taskScheduler.insertToConnections(con);
        }
        std::cout << "当前的use_count:" << connections_[0].use_count() << std::endl;
        std::cout << "地址是" << connections_[0].get() << std::endl;
        sleep(15);
        std::cout << "当前的use_count:" << connections_[0].use_count() << std::endl;
        std::cout << "地址是" << connections_[0].get() << std::endl;
        sleep(10000);
    }
    return 0;
}