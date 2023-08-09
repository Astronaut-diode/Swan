//
// Created by diode on 23-7-24.
//

#include "Address.h"

Address::Address(int port, std::string ip) {
    addr_.sin_family = PF_INET;
    addr_.sin_port = htons(port);
    port_ = port;
    ip_ = ip;
    addr_.sin_addr.s_addr = htonl(transform_ip_to_int(ip));
}

Address::~Address() {

}

sockaddr_in &Address::getAddr() {
    return addr_;
}

unsigned int Address::transform_ip_to_int(std::string ip) {       // 将ip(32位)转换为unsigned int的整数。
    unsigned int ret = 0;
    int dot_index = -1, bit = 0;
    while((dot_index = ip.rfind(".")) != -1) {
        ret = ret + (stoi(ip.substr(dot_index + 1)) << ((bit++) * 8));      // A.B.C.D 的结果是D << 24 + C << 16 + B << 8 + A << 0
        ip = ip.substr(0, dot_index);
    }
    ret = ret + (stoi(ip) << ((bit++) * 8));
    return ret;
}
