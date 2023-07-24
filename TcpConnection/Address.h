//
// Created by diode on 23-7-24.
//

#ifndef SWAN_ADDRESS_H
#define SWAN_ADDRESS_H

#include <netinet/in.h>
#include <cassert>

class Address {
public:  // 用于写typedef或者静态常量等。

private:  // 变量区域
    sockaddr_in addr_;
    int port_;
public:

private:  // 函数区域

public:
    Address() {};

    Address(int port);

    ~Address();

    sockaddr_in &getAddr();
};

#endif //SWAN_ADDRESS_H
