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
        FRIEND_LIST = 101,  // 好友列表
        GROUP_LIST = 102,  // 群组列表
        FRIEND_CHAT = 103,  // 请求获取好友之间的聊天历史记录。
        GROUP_CHAT = 104,  // 请求获取群组之间的聊天历史记录。
        SUCCESS = 200,  // 操作成功
        ADD_FRIEND_REQUEST = 201,  // 添加好友的请求。
        ADD_GROUP_REQUEST = 202,  // 添加群组的请求。
        CREATE_GROUP_REQUEST = 301,  // 创建群组的请求。
        FORCE_FRIEND_LIST = 401,  // 强制推送好友列表
        FORCE_GROUP_LIST = 402,  // 强制推送群组名单
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
    char serverKey_[30];
    int chatId_;  // 当前正在和谁聊天。
    bool isGroup_;  // 是不是群聊天。
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

    bool processAddFriendRequest();  // 处理发送添加请求，返回的bool代表请求是否发送成功。

    bool createGroup();  // 创建一个名字为groupName的群组，群主是本人。

    bool processAddGroupRequest();  // 发送添加群组的请求。

    bool chatWithFriend();  // 切换聊天状态为好友聊天状态。

    bool chatWithGroup();  // 切换聊天状态为群组聊天状态。
public:
    Request() {}

    Request(int clientFd);

    ~Request() {

    }

    bool receiveHTTPRequest();  // 接收来自HTTP的请求。

    void fetchHTTPInfo();  // 解析HTTP请求头。

    void parseStr(char *request);  // 构造需要返回的内容。

    int receiveWebSocketRequest();  // 接收来自WebSocket的信息。

    int fetch_websocket_info(char *msg);  // 分析信息

    int getUserId();

    bool pushAddFriendRequestMessage(int sourceId, int destId);  // 推送添加好友的信息。

    bool acceptOrRefuseAddFriendRequest();  // 接受或拒绝好友请求。

    bool pushAddGroupRequestMessage(int sourceId, int destId);  // 推送添加群组的信息。

    bool acceptOrRefuseAddGroupRequest();  // 接受或拒绝群组添加请求。

    void sendAllFriends();  // 推送所有好友的名单。

    void sendAllGroups();  // 推送所有群组的名单。

    bool sendMessage();  // 发送信息。

    void ForceUpdateSendAllFriends();  // 强制推送好友名单，前端必须更新。

    void ForceSendMessage(int sourceId, int destId);  // 强制刷新前端信息。

    void ForceSendGroupMessage(int groupId);  // 强制刷新前端的群组聊天记录。

    void ForceUpdateSendAllGroups();  // 强制推送好友名单，前端必须更新。
};

#endif //SWAN_REQUEST_H