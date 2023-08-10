//
// Created by diode on 23-7-26.
//


#include "Request.h"

Request::Request(int clientFd) : clientFd_(clientFd),
                                 response_(std::make_unique<Response>(clientFd_)),
                                 userId_(-1) {
    chatId_ = -1;
    isGroup_ = false;
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
    while (true) {
        readCount = recv(clientFd_, buffer_ + readNextIndex_, kReadBufferMaxLength - readNextIndex_, 0);
        if (readCount <= 0) {
            break;
        } else {
            readNextIndex_ += readCount;
        }
    }
    LOG << "接收到的HTTP请求" << buffer_;
    if (strstr(buffer_, "/favicon")) {  // 网页图标。
        response_->sendResourceFile(Response::SEND_TYPE::ICON, "/Resource/Img/favicon.ico");
        reset();
        return false;
    } else if (strstr(buffer_, "websocket")) {  // 请求建立连接。
        fetchHTTPInfo();  // 捕获请求头信息。
        parseStr(response_->buffer_);
        response_->establishWebSocketConnection();
        reset();
        return true;
    } else {  // 默认的初始连接。
        response_->sendResourceFile(Response::SEND_TYPE::HTML, "/Resource/Html/login.html");
        reset();
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

int Request::getUserId() {
    return userId_;
}

/**
 * 构建目标标签。
 * @param tag
 * @param content
 * @return
 */
std::string Request::createTagMessage(const std::string &tag, const std::string &content) {
    std::string ret = "<" + tag + ">" + content + "</" + tag + ">";
    return ret;
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
int Request::receiveWebSocketRequest() {
    int readCount = 0, ret = -1;
    while (true) {
        readCount = recv(clientFd_, buffer_ + readNextIndex_, kReadBufferMaxLength - readNextIndex_, 0);
        if (readCount == 0) {
            return -2;  // 代表websocket连接已经被关闭。
        } else if (readCount == -1) {
            break;
        } else {
            readNextIndex_ += readCount;
        }
    }

    fetch_websocket_info(buffer_);
    char log[4096];
    snprintf(log, 4096,
             "WEBSOCKET PROTOCOL  "
             "FIN: %d  "
             "OPCODE: %d  "
             "MASK: %d  "
             "PAYLOADLEN: %lu  "
             "PAYLOAD: %s",
             fin_, opcode_, mask_, payload_length_, payload_);
    LOG << clientFd_ << "接收到的信息" << log << "\n";
    std::string url = analysisTag(payload_, "url");  // 根据获取的url开始分流处理。
    memcpy(url_, url.c_str(), url.size());
    if (strcmp("Bad", url_) == 0) {  // 没有操作。

    } else if (strcmp("/register", url_) == 0) {  // 注册请求。
        if (processRegister()) {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "注册成功");
        } else {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "注册失败");
        }
    } else if (strcmp("/login", url_) == 0) {
        int userId = -1;
        if (processLogin(userId)) {
            std::string session = Redis::get_singleton_()->SessionExists(userId);
            std::string tags = createTagMessage("session", session);
            tags += createTagMessage("userId", std::to_string(userId));
            userId_ = userId;
            ret = userId_;  // 代表当前是登陆成功的请求。
            session_ = session;
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "登陆成功", tags);
            sendAllFriends();  // 发送好友名单。
            sendAllGroups();  // 发送群组名单。
            pushAddFriendRequestMessage(-1, -1);  // 发送当前用户所有待处理的添加好友请求。
            pushAddGroupRequestMessage(-1, -1);  // 发送当前用户所有待处理的添加群组请求。
        } else {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "登陆失败");
        }
    } else if (strcmp("/addFriendRequest", url_) == 0) {
        if (processAddFriendRequest()) {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "好友添加请求发送成功");
        } else {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "好友添加请求发送失败");
        }
    } else if (strcmp("/acceptOrRefuseAddFriendRequest", url_) == 0) {
        if (acceptOrRefuseAddFriendRequest()) {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "好友请求处理成功");
        } else {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "好友请求处理失败");
        }
    } else if (strcmp("/createGroup", url_) == 0) {
        if (createGroup()) {
            sendAllGroups();  // 群组建好的话，需要刷新群组名单。
            response_->sendWebSocketResponseBuffer(RET_CODE::CREATE_GROUP_REQUEST, "群组创建成功");
        } else {
            response_->sendWebSocketResponseBuffer(RET_CODE::CREATE_GROUP_REQUEST, "群组创建失败");
        }
    } else if (strcmp("/addGroupRequest", url_) == 0) {
        if (processAddGroupRequest()) {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "群组添加请求发送成功");
        } else {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "群组添加请求发送失败");
        }
    } else if (strcmp("/receiveAddGroupRequest", url_) == 0) {
        if (acceptOrRefuseAddGroupRequest()) {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "群组添加请求处理成功");
        } else {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "群组添加请求处理失败");
        }
    } else if (strcmp("/chatWithFriend", url_) == 0) {
        if (chatWithFriend()) {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "好友聊天列表切换成功");
        } else {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "好友聊天列表切换失败");
        }
    } else if (strcmp("/sendMessage", url_) == 0) {
        if (sendMessage()) {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "信息发送成功");
        } else {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "信息发送失败");
        }
    } else if (strcmp("/chatWithGroup", url_) == 0) {
        if (chatWithGroup()) {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "群组聊天列表切换成功");
        } else {
            response_->sendWebSocketResponseBuffer(RET_CODE::SUCCESS, "群组聊天列表切换失败");
        }
    }
    reset();
    return ret;
}


/**
 * 处理注册请求，返回的bool代表注册是否成功。
 * @return
 */
bool Request::processRegister() {
    std::string username = analysisTag(payload_, "username");
    std::string password = analysisTag(payload_, "password");
    bool ret = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->Register(username, password);
    return ret;
}

/**
 * 处理登录请求，返回的bool代表登录是否成功。
 * @return
 */
bool Request::processLogin(int &userId) {
    std::string username = analysisTag(payload_, "username");
    std::string password = analysisTag(payload_, "password");
    bool ret = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->Login(userId, username, password);
    return ret;
}

/**
 * 发送添加好友请求。
 * @param userId
 * @return
 */
bool Request::processAddFriendRequest() {
    std::string sourceId = analysisTag(payload_, "sourceId");
    std::string destId = analysisTag(payload_, "destId");
    std::string session = analysisTag(payload_, "session");
    if (session == session_ && stoi(sourceId) == userId_) {
        if (!MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->sendAddFriendRequest(stoi(sourceId),
                                                                                                        stoi(destId),
                                                                                                        false)) {
            return false;
        }
        Redis::get_singleton_()->publish("friendRequest", stoi(sourceId), stoi(destId));
        return true;
    }
    return false;
}

/**
 * 推送添加好友的信息。
 * @param sourceId
 * @param destId
 * @return
 */
bool Request::pushAddFriendRequestMessage(int sourceId, int destId) {
    std::string tags;
    // 找到所有想要添加我的人。
    std::vector<std::pair<int, std::string>> ids = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllWaitProcessFriendRequest(
            userId_);
    for (int i = 0; i < ids.size(); ++i) {
        tags += createTagMessage("friendRequest" + std::to_string(i), std::to_string(ids[i].first) + ":" +
                                                                      ids[i].second);  // 好友请求信息的格式friendRequest[0->n - 1]是tag，信息是id:name
    }
    tags += createTagMessage("nums", std::to_string(ids.size()));  // 有多少个好友请求。
    response_->sendWebSocketResponseBuffer(RET_CODE::ADD_FRIEND_REQUEST, "新的好友申请", tags);
    return true;
}

/**
 * 接受或拒绝好友请求。
 * @return
 */
bool Request::acceptOrRefuseAddFriendRequest() {
    std::string sourceId = analysisTag(payload_, "sourceId");
    std::string destId = analysisTag(payload_, "destId");
    std::string session = analysisTag(payload_, "session");
    std::string process = analysisTag(payload_, "process");  // 代表是否同意，yes或no
    if (session == session_ && stoi(destId) == userId_) {
        char *sql = new char[256];
        snprintf(sql, 256, "update friendRequest set processed = true where sourceId = %d and destId = %d;",
                 stoi(sourceId),
                 stoi(destId));
        MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->processSql(sql);
        if (process == "yes") {
            snprintf(sql, 256, "insert into friendRelation(sourceId, destId, lastReadTime) values (%d, %d, now());",
                     stoi(sourceId),
                     stoi(destId));
            MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->processSql(sql);
            snprintf(sql, 256, "insert into friendRelation(sourceId, destId, lastReadTime) values (%d, %d, now());",
                     stoi(destId),
                     stoi(sourceId));
            MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->processSql(sql);
            Redis::get_singleton_()->publish("friendList", stoi(destId), stoi(sourceId));  // 通知目标刷新好友列表。
            Redis::get_singleton_()->publish("friendList", stoi(sourceId), stoi(destId));  // 通知目标刷新好友列表。
        }
        delete[] sql;
        return true;
    }
    return false;
}

/**
 * 推送所有好友的名单（不强制修改前端版本）。
 */
void Request::sendAllFriends() {
    std::vector<std::pair<int, std::string>> ids = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllFriendsId(
            userId_);
    std::string tags;
    for (int i = 0; i < ids.size(); ++i) {
        // 进行额外查询，看看他给我发了多少消息还没看。
        int count = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllUnreadFriendMessageCount(
                ids[i].first, userId_);
        tags += createTagMessage("friend" + std::to_string(i), std::to_string(ids[i].first) + ":" +
                                                               ids[i].second + ":" + std::to_string(
                count));  // 好友信息的格式friend[0->n - 1]是tag，信息是id:name:未读消息数量。
    }
    tags += createTagMessage("nums", std::to_string(ids.size()));  // 有多少个好友。
    response_->sendWebSocketResponseBuffer(RET_CODE::FRIEND_LIST, "好友列表", tags);
}

/**
 * 创建一个名字为groupName的群组，群主是本人。
 * @return
 */
bool Request::createGroup() {
    std::string sourceId = analysisTag(payload_, "sourceId");
    std::string session = analysisTag(payload_, "session");
    std::string groupName = analysisTag(payload_, "groupName");
    if (session == session_ && stoi(sourceId) == userId_) {
        return MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->createGroup(userId_, groupName);
    }
    return false;
}

/**
 * 推送所有群组的名单。
 */
void Request::sendAllGroups() {
    std::vector<std::pair<int, std::string>> ids = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllGroups(
            userId_);
    std::string tags;
    for (int i = 0; i < ids.size(); ++i) {
        // 进行额外查询，看看他给我发了多少消息还没看。
        int count = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllUnreadGroupMessageCount(
                ids[i].first, userId_);
        tags += createTagMessage("group" + std::to_string(i), std::to_string(ids[i].first) + ":" +
                                                              ids[i].second + ":" + std::to_string(
                count));  // 群组信息的格式group[0->n - 1]是tag，信息是id:name:count
    }
    tags += createTagMessage("nums", std::to_string(ids.size()));  // 有多少个群组。
    response_->sendWebSocketResponseBuffer(RET_CODE::GROUP_LIST, "群组列表", tags);
}

/**
 * 发送添加群组请求。
 * @param userId
 * @return
 */
bool Request::processAddGroupRequest() {
    std::string sourceId = analysisTag(payload_, "sourceId");
    std::string groupId = analysisTag(payload_, "destId");
    std::string session = analysisTag(payload_, "session");
    if (session == session_ && stoi(sourceId) == userId_) {
        if (!MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->sendAddGroupRequest(stoi(groupId),
                                                                                                       stoi(sourceId),
                                                                                                       false)) {
            return false;
        }
        int masterId = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findUserIdByGroupId(
                std::stoi(groupId));
        Redis::get_singleton_()->publish("groupRequest", stoi(sourceId), masterId);
        return true;
    }
    return false;
}

/**
 * 推送添加群组的信息。
 * @param sourceId
 * @param destId
 * @return
 */
bool Request::pushAddGroupRequestMessage(int sourceId, int destId) {
    std::string tags;
    // 找到所有想要添加我的群的人。
    std::vector<std::vector<std::string>> ids = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllWaitProcessGroupRequest(
            userId_);
    for (int i = 0; i < ids.size(); ++i) {
        tags += createTagMessage("groupRequest" + std::to_string(i),
                                 ids[i][0] + ":" + ids[i][1] + ":" + ids[i][2] + ":" +
                                 ids[i][3] + ":" +
                                 ids[i][4]);  // 群组请求信息的格式groupRequest[0->n - 1]是tag，信息是添加请求的id，请求者名字:请求者id:群组ID：群组名字。
    }
    tags += createTagMessage("nums", std::to_string(ids.size()));  // 有多少个群组请求。
    response_->sendWebSocketResponseBuffer(RET_CODE::ADD_GROUP_REQUEST, "新的群组申请", tags);
    return true;
}


/**
 * 接受或拒绝好友请求。
 * @return
 */
bool Request::acceptOrRefuseAddGroupRequest() {
    std::string sourceId = analysisTag(payload_, "sourceId");
    std::string groupId = analysisTag(payload_, "destId");
    std::string masterId = analysisTag(payload_, "masterId");
    std::string session = analysisTag(payload_, "session");
    std::string process = analysisTag(payload_, "process");  // 代表是否同意，yes或no
    if (session == session_ && stoi(masterId) == userId_) {
        char *sql = new char[256];
        snprintf(sql, 256, "update groupRequest set processed = true where sourceId = %d and destId = %d;",
                 stoi(groupId), stoi(sourceId)); // 两个参数搞反了。
        MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->processSql(sql);
        if (process == "yes") {
            snprintf(sql, 256, "insert into groupRelation(sourceId, destId, lastReadTime) values (%d, %d, now());",
                     stoi(groupId),
                     stoi(sourceId));
            MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->processSql(sql);
            Redis::get_singleton_()->publish("groupList", -1, stoi(sourceId));  // 通知目标刷新群组列表。
        }
        delete[] sql;
        return true;
    }
    return false;
}

/**
 * 切换聊天状态为好友聊天状态。
 * @return
 */
bool Request::chatWithFriend() {
    std::string sourceId = analysisTag(payload_, "sourceId");  // 对方
    chatId_ = stoi(sourceId);
    isGroup_ = false;
    std::string destId = analysisTag(payload_, "destId");  // 我
    std::string session = analysisTag(payload_, "session");
    if (session == session_ && stoi(destId) == userId_) {
        // 按照(对方,我)找出最后的读取时间，然后取出所有大于这个时间的信息，取出每一个结果。分别是聊天记录id，请求者id，请求者name,信息内容，时间。
        std::vector<std::vector<std::string>> memo = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllUnreadFriendMessage(
                stoi(sourceId), stoi(destId));  // 聊天记录。
        sendAllFriends();  // 发送好友名单(可以改变未读消息数量。)
        std::string tags;
        for (int i = 0; i < memo.size(); ++i) {
            tags += createTagMessage("friendChat" + std::to_string(i),
                                     memo[i][0] + "$:$" + memo[i][1] + "$:$" + memo[i][2] + "$:$" + memo[i][3] + "$:$" +
                                     memo[i][4]);  // 避免切割到时间，使用$:$
        }
        tags += createTagMessage("nums", std::to_string(memo.size()));  // 有多少条聊天记录。
        tags += createTagMessage("username",
                                 MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findUserNameByUserId(
                                         stoi(sourceId)));
        response_->sendWebSocketResponseBuffer(RET_CODE::FRIEND_CHAT, "好友之间的聊天记录", tags);
        return true;
    }
    return false;
}

/**
 * 发送信息
 * @return
 */
bool Request::sendMessage() {
    std::string session = analysisTag(payload_, "session");
    std::string sourceId = analysisTag(payload_, "sourceId");
    std::string destId = analysisTag(payload_, "destId");
    std::string isGroup = analysisTag(payload_, "isGroup");
    std::string message = analysisTag(payload_, "message");
    if (session == session_ && stoi(sourceId) == userId_) {
        if (isGroup == "false") {
            char *sql = new char[1024];
            snprintf(sql, 1024,
                     "insert into friendMessage(sourceId, destId, content, sendTime) values (%d, %d, ?, now());",
                     stoi(sourceId), stoi(destId));  // 记录消息内容。
            if (!MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->sendMessage(sql, message)) {
                delete[] sql;
                return false;
            }
            Redis::get_singleton_()->publish("friendMessage", stoi(sourceId), stoi(destId));  // 通知目标刷新好友信息。
            delete[] sql;
            ForceSendMessage(stoi(destId), stoi(sourceId));  // 强制更新前端聊天框内容。
            return true;
        } else {
            char *sql = new char[1024];
            snprintf(sql, 1024,
                     "insert into groupMessage(sourceId, innerSourceId, content, sendTime) values (%d, %d, ?, now());",
                     stoi(destId), stoi(sourceId));  // 记录消息内容(此时的destId就是groupId)
            if (!MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->sendMessage(sql, message)) {
                delete[] sql;
                return false;
            };
            std::vector<int> members = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllMemberInGroup(
                    stoi(destId));
            for (int member: members) {
                Redis::get_singleton_()->publish("groupMessage", stoi(destId), member);  // 群组通知全体成员有信息。
            }
            delete[] sql;
            return true;
        }
    }
    return false;
}


/**
 * 推送所有好友的名单（强制修改前端版本）。
 */
void Request::ForceUpdateSendAllFriends() {
    std::vector<std::pair<int, std::string>> ids = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllFriendsId(
            userId_);
    std::string tags;
    for (int i = 0; i < ids.size(); ++i) {
        // 进行额外查询，看看他给我发了多少消息还没看。
        int count = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllUnreadFriendMessageCount(
                ids[i].first, userId_);
        tags += createTagMessage("friend" + std::to_string(i), std::to_string(ids[i].first) + ":" +
                                                               ids[i].second + ":" + std::to_string(
                count));  // 好友信息的格式friend[0->n - 1]是tag，信息是id:name:未读消息数量。
    }
    tags += createTagMessage("nums", std::to_string(ids.size()));  // 有多少个好友。
    response_->sendWebSocketResponseBuffer(RET_CODE::FORCE_FRIEND_LIST, "好友列表", tags);
}

void Request::ForceSendMessage(int sourceId, int destId) {
    // 按照(对方,我)找出最后的读取时间，然后取出所有大于这个时间的信息，取出每一个结果。分别是聊天记录id，请求者id，请求者name,信息内容，时间。
    std::vector<std::vector<std::string>> memo = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllUnreadFriendMessage(
            sourceId, destId);  // 聊天记录。
    sendAllFriends();  // 发送好友名单(可以改变未读消息数量。)
    std::string tags;
    for (int i = 0; i < memo.size(); ++i) {
        tags += createTagMessage("friendChat" + std::to_string(i),
                                 memo[i][0] + "$:$" + memo[i][1] + "$:$" + memo[i][2] + "$:$" + memo[i][3] + "$:$" +
                                 memo[i][4]);  // 避免切割到时间，使用$:$
    }
    tags += createTagMessage("username",
                             MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findUserNameByUserId(
                                     sourceId));
    tags += createTagMessage("nums", std::to_string(memo.size()));  // 有多少条聊天记录。
    response_->sendWebSocketResponseBuffer(RET_CODE::FRIEND_CHAT, "好友之间的聊天记录", tags);
}


/**
 * 切换聊天状态为群组聊天状态。
 * @return
 */
bool Request::chatWithGroup() {
    std::string sourceId = analysisTag(payload_, "groupId");  // 群组id
    chatId_ = stoi(sourceId);
    isGroup_ = true;
    std::string destId = analysisTag(payload_, "destId");  // 我
    std::string session = analysisTag(payload_, "session");
    if (session == session_ && stoi(destId) == userId_) {
        // 按照(对方,我)找出最后的读取时间，取出每一个结果。分别是聊天记录id，请求者id，请求者name,信息内容，时间。
        std::vector<std::vector<std::string>> memo = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllGroupMessage(
                stoi(sourceId), stoi(destId));  // 聊天记录。
        sendAllGroups();  // 发送群组名单(可以改变未读消息数量。)
        std::string tags;
        for (int i = 0; i < memo.size(); ++i) {
            tags += createTagMessage("groupChat" + std::to_string(i),
                                     memo[i][0] + "$:$" + memo[i][1] + "$:$" + memo[i][2] + "$:$" + memo[i][3] + "$:$" +
                                     memo[i][4] + "$:$" + memo[i][5]);  // 避免切割到时间，使用$:$
        }
        tags += createTagMessage("nums", std::to_string(memo.size()));  // 有多少条聊天记录。
        tags += createTagMessage("groupName",
                                 MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findGroupNameByGroupId(
                                         stoi(sourceId)));
        response_->sendWebSocketResponseBuffer(RET_CODE::GROUP_CHAT, "群组的聊天记录", tags);
        return true;
    }
    return false;
}


/**
 * 强制发送消息，更新前端的群组聊天界面。
 * @param groupId
 */
void Request::ForceSendGroupMessage(int groupId) {
    // 按照(对方,我)找出最后的读取时间，取出每一个结果。
    std::vector<std::vector<std::string>> memo = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllGroupMessage(
            groupId, userId_);  // 聊天记录。
    sendAllGroups();  // 发送群组名单(可以改变未读消息数量。)
    std::string tags;
    for (int i = 0; i < memo.size(); ++i) {
        tags += createTagMessage("groupChat" + std::to_string(i),
                                 memo[i][0] + "$:$" + memo[i][1] + "$:$" + memo[i][2] + "$:$" + memo[i][3] + "$:$" +
                                 memo[i][4] + "$:$" + memo[i][5]);  // 避免切割到时间，使用$:$
    }
    tags += createTagMessage("nums", std::to_string(memo.size()));  // 有多少条聊天记录。
    tags += createTagMessage("groupName",
                             MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findGroupNameByGroupId(
                                     groupId));
    response_->sendWebSocketResponseBuffer(RET_CODE::GROUP_CHAT, "群组的聊天记录", tags);
}

/**
 * 推送所有群组的名单（强制修改前端版本）。
 */
void Request::ForceUpdateSendAllGroups() {
    std::vector<std::pair<int, std::string>> ids = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllGroups(
            userId_);
    std::string tags;
    for (int i = 0; i < ids.size(); ++i) {
        // 进行额外查询，看看他给我发了多少消息还没看。
        int count = MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_()->findAllUnreadGroupMessageCount(
                ids[i].first, userId_);
        tags += createTagMessage("group" + std::to_string(i), std::to_string(ids[i].first) + ":" +
                                                              ids[i].second + ":" + std::to_string(
                count));  // 群组信息的格式group[0->n - 1]是tag，信息是id:name:count
    }
    tags += createTagMessage("nums", std::to_string(ids.size()));  // 有多少个群组。
    response_->sendWebSocketResponseBuffer(RET_CODE::FORCE_GROUP_LIST, "群组列表", tags);
}
