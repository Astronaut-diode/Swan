//
// Created by diode on 23-7-23.
// 连接类，暂时为空，仅仅当作占位符。

#ifndef SWAN_TCPCONNECTION_H
#define SWAN_TCPCONNECTION_H

#include <iostream>

class TcpConnection {
public:  // 用于写typedef或者静态常量等。

private:  // 变量区域

public:

private:  // 函数区域

public:
    TcpConnection() {
        std::cout << "TcpConnection()" << std::endl;
    };

    ~TcpConnection() {
        std::cout << "~TcpConnection()" << std::endl;
    };

    void stop() {
        std::cout << "准备关停当前对象，并析构" << std::endl;
    }
};

#endif //SWAN_TCPCONNECTION_H
