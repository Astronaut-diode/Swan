//
// Created by diode on 23-7-26.
//

#ifndef SWAN_REQUEST_H
#define SWAN_REQUEST_H

#include <cstring>
#include <memory>
#include <semaphore.h>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../Logger/LogStream.h"
#include "Response.h"
#include "../Utils/sha1.h"
#include "../Utils/base64.h"
#include "../MysqlConnectionPool/MysqlConnectionPool.h"
#include "../Redis/Redis.h"

#define MAGIC_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

typedef std::map<std::string, std::string> HEADER_MAP;

class Request {
public:  // 用于写typedef或者静态常量等。
    static const int kReadBufferMaxLength = 4 * 1024;
    enum RET_CODE {
        SUCCESS = 200,  // 操作成功
    };
private:  // 变量区域
    int clientFd_;  // 对方的文件描述符。
    char buffer_[kReadBufferMaxLength];
    int readNextIndex_;  // 下一次加载内容的下标。
    std::unique_ptr<Response> response_;
    HEADER_MAP headerMap_;

    uint8_t fin_;
    uint8_t opcode_;
    uint8_t mask_;
    uint8_t masking_key_[4];
    uint64_t payload_length_;
    char payload_[2048];


    char url_[128];  // websocket发起的请求地址。
    int userId_;  // 用户的id。
    std::string session_;  // 对应的session。
public:
    char serverKey_[20];
private:  // 函数区域
    void reset();

    int fetch_fin(char *msg, int &pos);

    int fetch_opcode(char *msg, int &pos);

    int fetch_mask(char *msg, int &pos);

    int fetch_masking_key(char *msg, int &pos);

    int fetch_payload_length(char *msg, int &pos);

    int fetch_payload(char *msg, int &pos);

    std::string analysisTag(char *buffer, std::string tag);

    std::string createTagMessage(const std::string &tag, const std::string &content);  // 构建目标标签。

    bool processRegister();  // 处理注册请求，返回的bool代表注册是否成功。

    bool processLogin(int &userId);  // 处理登录请求，返回的bool代表登录是否成功。
public:
    Request() {}

    Request(int clientFd);

    ~Request() {

    }

    bool receiveHTTPRequest();  // 接收来自HTTP的请求。

    void fetchHTTPInfo();  // 解析HTTP请求头。

    void parseStr(char *request);  // 构造需要返回的内容。

    void receiveWebSocketRequest();  // 接收来自WebSocket的信息。

    int fetch_websocket_info(char *msg);  // 分析信息

    int getUserId();
};

#endif //SWAN_REQUEST_H
