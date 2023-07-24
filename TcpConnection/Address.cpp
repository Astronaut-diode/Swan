//
// Created by diode on 23-7-24.
//

#include "Address.h"

Address::Address(int port) {
    addr_.sin_family = PF_INET;
    addr_.sin_port = htons(port);
    port_ = port;
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
}

Address::~Address() {

}

sockaddr_in &Address::getAddr() {
    return addr_;
}