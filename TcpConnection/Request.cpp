//
// Created by diode on 23-7-26.
//


#include "Request.h"

Request::Request(int clientFd) : clientFd_(clientFd),
                                 response_(std::make_unique<Response>(clientFd_)) {
    reset();
}

/**
 * 初始化内容。
 */
void Request::reset() {
    memset(buffer_, '\0', kReadBufferMaxLength);
    readNextIndex_ = 0;
    headerMap_.clear();
    fin_ = 0;
    opcode_ = 0;
    mask_ = 0;
    memset(masking_key_, 0, sizeof(masking_key_));
    payload_length_ = 0;
    memset(payload_, 0, sizeof(payload_));
    memset(serverKey_, 0, sizeof(serverKey_));
    memset(url_, 0, sizeof(url_));
}

/**
 * 接收来自HTTP的请求。
 */
bool Request::receiveHTTPRequest() {
    int readCount = 0;
    while ((readCount = recv(clientFd_, buffer_ + readNextIndex_, kReadBufferMaxLength - readNextIndex_, 0)) != -1);
    if (strstr(buffer_, "/favicon")) {  // 网页图标。
        response_->sendResourceFile(Response::SEND_TYPE::ICON, "/Resource/Img/favicon.ico");
        return false;
    } else if (strstr(buffer_, "websocket")) {  // 请求建立连接。
        fetchHTTPInfo();  // 捕获请求头信息。
        parseStr(response_->buffer_);
        response_->establishWebSocketConnection();
        return true;
    } else {  // 默认的初始连接。
        response_->sendResourceFile(Response::SEND_TYPE::HTML, "/Resource/Html/login.html");
        return false;
    }
}

/**
 * 截取HTTP请求头信息。
 */
void Request::fetchHTTPInfo() {
    std::istringstream s(buffer_);
    std::string request;

    std::getline(s, request);
    if (request[request.size() - 1] == '\r') {
        request.erase(request.end() - 1);
    }

    std::string header;
    std::string::size_type end;

    while (std::getline(s, header) && header != "\r") {
        if (header[header.size() - 1] != '\r') {
            continue; //end
        } else {
            header.erase(header.end() - 1);    //remove last char
        }

        end = header.find(": ", 0);
        if (end != std::string::npos) {
            std::string key = header.substr(0, end);
            std::string value = header.substr(end + 2);
            headerMap_[key] = value;
        }
    }
}


void Request::parseStr(char *request) {
    strcat(request, "HTTP/1.1 101 Switching Protocols\r\n");
    strcat(request, "Connection: upgrade\r\n");
    strcat(request, "Sec-WebSocket-Accept: ");
    std::string server_key = headerMap_["Sec-WebSocket-Key"];
    server_key += MAGIC_KEY;

    SHA1 sha;
    unsigned int message_digest[5];
    sha.Reset();
    sha << server_key.c_str();

    sha.Result(message_digest);
    for (int i = 0; i < 5; i++) {
        message_digest[i] = htonl(message_digest[i]);
    }
    server_key = base64_encode(reinterpret_cast<const unsigned char *>(message_digest), 20);  // 这个可以直接当作session使用。
    strcpy(serverKey_, server_key.c_str());
    server_key += "\r\n";
    strcat(request, server_key.c_str());
    strcat(request, "Upgrade: websocket\r\n\r\n");
}


int Request::fetch_websocket_info(char *msg) {
    int pos = 0;
    fetch_fin(msg, pos);
    fetch_opcode(msg, pos);
    fetch_mask(msg, pos);
    fetch_payload_length(msg, pos);
    fetch_masking_key(msg, pos);
    return fetch_payload(msg, pos);
}

int Request::fetch_fin(char *msg, int &pos) {
    fin_ = (unsigned char) msg[pos] >> 7;
    return 0;
}

int Request::fetch_opcode(char *msg, int &pos) {
    opcode_ = msg[pos] & 0x0f;
    pos++;
    return 0;
}

int Request::fetch_mask(char *msg, int &pos) {
    mask_ = (unsigned char) msg[pos] >> 7;
    return 0;
}

int Request::fetch_masking_key(char *msg, int &pos) {
    if (mask_ != 1)
        return 0;
    for (int i = 0; i < 4; i++)
        masking_key_[i] = msg[pos + i];
    pos += 4;
    return 0;
}

int Request::fetch_payload_length(char *msg, int &pos) {
    payload_length_ = msg[pos] & 0x7f;
    pos++;
    if (payload_length_ == 126) {
        uint16_t length = 0;
        memcpy(&length, msg + pos, 2);
        pos += 2;
        payload_length_ = ntohs(length);
    } else if (payload_length_ == 127) {
        uint32_t length = 0;
        memcpy(&length, msg + pos, 4);
        pos += 4;
        payload_length_ = ntohl(length);
    }
    return 0;
}

int Request::fetch_payload(char *msg, int &pos) {
    memset(payload_, 0, sizeof(payload_));
    if (mask_ != 1) {
        memcpy(payload_, msg + pos, payload_length_);
    } else {
        for (uint i = 0; i < payload_length_; i++) {
            int j = i % 4;
            payload_[i] = msg[pos + i] ^ masking_key_[j];
        }
    }
    pos += payload_length_;
    return 0;
}

/**
 * 从发送的信息中找出对应的url请求目标。
 * @param buffer  原始的内容字符串。
 * @param tag  等待获取的标签
 */
std::string Request::analysisTag(char *buffer, std::string tag) {
    std::string leftTag = "<" + tag + ">";
    const char *start = strstr(buffer, leftTag.c_str());  // 因为url一定是第一个标签，所以可以直接先获取第一个>。
    if (start == nullptr) {  // 代表内容格式不正确。
        return "Bad";
    }
    std::string rightTag = "</" + tag + ">";
    const char *end = strstr(buffer, rightTag.c_str());  // 找出start右边的第一个<。
    char content[1024]{'\0'};
    strncpy(content, start + leftTag.size(), end - start - leftTag.size());
    return content;
}

/**
 * 接收来自WebSocket的信息。
 */
void Request::receiveWebSocketRequest() {
    int readCount = 0;
    while ((readCount = recv(clientFd_, buffer_ + readNextIndex_, kReadBufferMaxLength - readNextIndex_, 0)) != -1);
    fetch_websocket_info(buffer_);
    char log[1024];
    snprintf(log, 1024,
             "WEBSOCKET PROTOCOL  "
             "FIN: %d  "
             "OPCODE: %d  "
             "MASK: %d  "
             "PAYLOADLEN: %d  "
             "PAYLOAD: %s",
             fin_, opcode_, mask_, payload_length_, payload_);
    LOG << clientFd_ << "接收到的信息" << log << "\n";
    std::string url = analysisTag(payload_, "url");  // 根据获取的url开始分流处理。
    memcpy(url_, url.c_str(), url.size());
    if (strcmp("Bad", url_) == 0) {  // 没有操作。

    } else if (strcmp("/register", url_) == 0) {  // 注册请求。
        std::string username = analysisTag(payload_, "username");
        std::string password = analysisTag(payload_, "password");
        response_->sendWebSocketResponseBuffer(RET_CODE::REGISTER_SUCCESS, "注册成功");
    }
    reset();
}
