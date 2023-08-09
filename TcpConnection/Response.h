//
// Created by diode on 23-7-26.
//

#ifndef SWAN_RESPONSE_H
#define SWAN_RESPONSE_H
#include <cstdarg>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <cstring>
#include <sys/socket.h>
#include "../Config/Config.h"
#include "../Logger/LogStream.h"

class Response {
public:  // 用于写typedef或者静态常量等。
    static const int kWriteBufferMaxLength = 4 * 1024;
    static const int kFilePathMaxLength = 256;
    enum SEND_TYPE {  // 发送的类型。
        ICON = 0,
        HTML
    };
private:  // 变量区域
    int clientFd_;  // 对方的文件描述符。
    int writeNextIndex_;  // 下一次写内容的下标。
    char response_file_path_[kFilePathMaxLength];  // 响应的HTML页面的完全路径。
    char *map_response_file_address_;  // 将页面加载到内存中去。
    struct stat response_file_stat_;  // 响应文件的详情。

    struct iovec iov_[2];
    int iov_count_;
    unsigned byte_to_send;  // 等待发送的。
    unsigned byte_have_send;  // 已经发送的。
public:
    char buffer_[kWriteBufferMaxLength];

private:  // 函数区域
    void reset();

    void AddResponse(const char *format, ...);

    void AddLine(int response_code, const char *response_phrase);

    void AddHeader(int content_length, const std::string &content_type);

    void AddBody(const char *body);

    void UpdateIov(int tmp, char *starts[]);
public:
    Response() {};

    Response(int clientFd);

    ~Response() {

    };

    void sendResourceFile(Response::SEND_TYPE sendType, const char *fileName);  // 发送资源文件，不管是图片还是页面。

    void establishWebSocketConnection();  // 建立WebSocket连接。

    void sendWebSocketResponseBuffer(int status, const std::string &message, const std::string &tags = "");  // 构建wbeSocket返回的信息。

    std::string createTagMessage(const std::string &tag, const std::string &content);  // 构建目标标签。
};

#endif //SWAN_RESPONSE_H
