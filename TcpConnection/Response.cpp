//
// Created by diode on 23-7-26.
//

#include "Response.h"

#include <cstring>

Response::Response(int clientFd) : clientFd_(clientFd) {

}

void Response::reset() {
    memset(buffer_, '\0', kWriteBufferMaxLength);
    memset(response_file_path_, '\0', kFilePathMaxLength);
    writeNextIndex_ = 0;
    iov_count_ = 0;
    byte_to_send = 0;
    byte_have_send = 0;
}


/**
 * 发送资源文件，不管是图片还是页面。
 */
void Response::sendResourceFile(Response::SEND_TYPE sendType, const char *fileName) {
    getcwd(response_file_path_, kFilePathMaxLength);
    stat(strcat(response_file_path_, fileName), &response_file_stat_);
    int fd = open(response_file_path_, O_RDONLY);
    map_response_file_address_ = (char *) mmap(0, response_file_stat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);  // 映射目标文件。

    const char *status_phrase{"OK\0"};  // 开始构造需要发送的内容。
    AddLine(200, status_phrase);
    if (sendType == SEND_TYPE::HTML) {
        AddHeader(response_file_stat_.st_size, "text/html;utf-8");
    } else if (sendType == SEND_TYPE::ICON) {
        AddHeader(response_file_stat_.st_size, "image/x-icon");
    }
    iov_[0].iov_base = buffer_;
    iov_[0].iov_len = writeNextIndex_;
    iov_[1].iov_base = map_response_file_address_;
    iov_[1].iov_len = response_file_stat_.st_size;
    iov_count_ = 2;
    byte_to_send = writeNextIndex_ + response_file_stat_.st_size;

    int tmp = 0;  // 开始发送。
    while (true) {
        tmp = writev(clientFd_, iov_, iov_count_);
        if (tmp == -1 && byte_to_send > 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            } else {
                break;
            }
        }
        if (tmp > 0) {
            byte_have_send = byte_have_send + tmp;
            byte_to_send = byte_to_send - tmp;
        }
        char *starts[]{buffer_, map_response_file_address_};
        UpdateIov(tmp, starts);  // 更新缓冲区。
        if (byte_to_send <= 0) {  // 发完了。
            if (iov_count_ > 1) {  // 需要解除内存中的映射。
                munmap(map_response_file_address_, response_file_stat_.st_size);
                map_response_file_address_ = 0;
            }
            reset();
            break;
        }
    }
}

/**
 * 往buffer缓冲中写内容。
 * @param format
 * @param ...
 * @return
 */
void Response::AddResponse(const char *format, ...) {
    va_list va_list;
    va_start(va_list, format);
    int len = vsnprintf(buffer_ + writeNextIndex_, kWriteBufferMaxLength - writeNextIndex_ - 1,
                        format, va_list);
    va_end(va_list);
    writeNextIndex_ = writeNextIndex_ + len;
}

/**
 * 写入line。
 * @param response_code
 * @param response_phrase
 * @return
 */
void Response::AddLine(int response_code, const char *response_phrase) {
    AddResponse("%s %d %s\r\n", "HTTP/1.1", response_code, response_phrase);
}

/**
 * 新增响应头
 * @param content_length
 * @return
 */
void Response::AddHeader(int content_length, const std::string &content_type) {
    AddResponse("Content-Length: %d\r\n", content_length);
    AddResponse("Content-Type: %s\r\n", content_type.c_str());
    AddResponse("Connection: %s\r\n", "keep-alive");  // 待会可以试试看，是否需要改为非长连接。
    AddResponse("\r\n");
}


/**
 * 新增响应体。
 * @param body
 * @return
 */
void Response::AddBody(const char *body) {
    AddResponse("%s", body);
}


/**
 * 根据发送的内容，修改iov，因为iov不仅仅只有一个或者两个了，现在可以多个。
 * @param tmp
 * @param 多个缓存的起始地址
 */
void Response::UpdateIov(int tmp, char *starts[]) {
    for (int i = 0; i < iov_count_; ++i) {
        if (tmp > 0 && iov_[i].iov_len > 0) {
            unsigned used = (iov_[i].iov_len < tmp) ? iov_[i].iov_len : tmp;
            iov_[i].iov_len -= used;
            iov_[i].iov_base = (char *) iov_[i].iov_base + used;  // 这里不能直接使用starts[i]，starts[i]其实从头到尾都没有发生过改变。
            tmp -= used;
        }
    }
}

/**
 * 建立WebSocket连接。
 */
void Response::establishWebSocketConnection() {
    write(clientFd_, buffer_, strlen(buffer_));
}