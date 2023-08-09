//
// Created by diode on 23-7-24.
//

#ifndef SWAN_ADDRESS_H
#define SWAN_ADDRESS_H

#include <string>
#include <netinet/in.h>
#include <cassert>

class Address {
public:  // 用于写typedef或者静态常量等。

private:  // 变量区域
    sockaddr_in addr_;
    std::string ip_;
    int port_;
public:

private:  // 函数区域

public:
    Address() {};

    Address(int port, std::string ip);

    ~Address();

    sockaddr_in &getAddr();

    unsigned int transform_ip_to_int(std::string ip);
};

#endif //SWAN_ADDRESS_H
