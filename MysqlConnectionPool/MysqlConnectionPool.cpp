//
//  Created by diode on 23-6-25.
// 数据库连接池的实现类
//

#include "MysqlConnectionPool.h"

MysqlConnectionPool *MysqlConnectionPool::mysql_connection_pool_singleton_instance_ = nullptr;  // 数据库连接池的单例对象以及对应的静态互斥锁。
pthread_mutex_t MysqlConnectionPool::singleton_instance_mutex_ = PTHREAD_MUTEX_INITIALIZER;

// 完成所有的初始化工作。
MysqlConnectionPool::MysqlConnectionPool() {  // 构造函数，通过Config的单例模型获取一些必要参数
    host_ = Config::get_singleton_()->mysql_host_;
    port_ = Config::get_singleton_()->mysql_port_;
    username_ = Config::get_singleton_()->mysql_username_;
    password_ = Config::get_singleton_()->mysql_password_;
    database_name_ = Config::get_singleton_()->mysql_database_name_;
    connection_max_num_ = Config::get_singleton_()->mysql_connection_max_num_;
    used_connection_num_ = 0;
    pthread_mutex_init(&mysql_pool_mutex_, nullptr);
    for (int i = 0; i < this->connection_max_num_; ++i) {
        MYSQL *conn = nullptr;
        conn = mysql_init(conn);
        assert(conn != nullptr);  // 如果为空，那就说明初始化存在问题。
        mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8");  // 设置字符集
        conn = mysql_real_connect(conn, host_.c_str(), username_.c_str(),
                                  password_.c_str(),
                                  database_name_.c_str(), stoi(port_),
                                  nullptr, 0);  // 与数据库建立连接
        assert(conn != nullptr);
        this->mysql_pool_.emplace_back(conn);
    }
    InitTable();  // 开始初始化表
}


/**
 * 释放所有的空闲连接。
 */
MysqlConnectionPool::~MysqlConnectionPool() {
    pthread_mutex_lock(&mysql_pool_mutex_);
    if (!mysql_pool_.empty()) {
        for (std::vector<MYSQL *>::iterator ite = mysql_pool_.begin(); ite != mysql_pool_.end(); ++ite) {
            MYSQL *conn = *ite;  // 这里是因为使用的迭代器，所以需要解引用，如果是增强for循环则不需要。
            mysql_close(conn);
        }
    }
    pthread_mutex_unlock(&mysql_pool_mutex_);
    pthread_mutex_destroy(&singleton_instance_mutex_);
}


bool MysqlConnectionPool::InitTable() {  // 初始化所有可能会使用到的表。
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {  // 一开始的时候一定是空闲状态，所以可以初始化表。
        std::string sqls[] = {
                "create table if not exists user(userId   int primary key auto_increment comment '用户id',username varchar(128) comment '用户账户',password varchar(128) comment '用户密码') comment '用户表' character set utf8mb4 collate utf8mb4_unicode_ci;",
                "create table if not exists `group`(groupId   int primary key auto_increment comment '群的id',groupName varchar(128) comment '群组的名字',masterId  int comment '群主的用户id') comment '群组表' character set utf8mb4 collate utf8mb4_unicode_ci;",
                "create table if not exists friendRelation(friendRelationId int primary key auto_increment comment '一组关系的主键',sourceId         int comment '出发点（好友id）',destId           int comment '目标点',lastReadTime     timestamp comment '最后一次dest_id用户接收source_id(仅仅是用户)信息的时间。') comment '关系表，仅记录好友关系，以及该用户接收该关系发送过来信息的最后一次时间。' character set utf8mb4 collate utf8mb4_unicode_ci;",
                "create table if not exists groupRelation(groupRelationId int primary key auto_increment comment '一组关系的主键',sourceId        int comment '出发点（群组id）',destId          int comment '目标点',lastReadTime    timestamp comment '最后一次dest_id用户接收source_id(仅仅是群组)信息的时间。') comment '关系表，仅记录群组关系以及当前用户接收该群消息的最后一次时间。' character set utf8mb4 collate utf8mb4_unicode_ci;",
                "create table if not exists friendMessage(friendMessageId int primary key auto_increment comment '用户聊天记录的id',sourceId        int comment '发送信息的人',destId          int comment '接收信息的人',content         varchar(1024) comment '发送的信息内容，长度最长是1024bytes',sendTime        timestamp comment '信息的发送时间，用于让目标用户找到未读信息') comment '用户聊天记录表' character set utf8mb4 collate utf8mb4_unicode_ci;",
                "create table if not exists groupMessage(groupMessageId int primary key auto_increment comment '群组聊天记录的id',sourceId       int comment '发送信息的群组',innerSourceId  int comment '发送信息的具体用户的id',content        varchar(1024) comment '发送的信息内容，长度最长是1024bytes',sendTime       timestamp comment '信息的发送时间，用于找到未读信息') comment '群组聊天记录表。' character set utf8mb4 collate utf8mb4_unicode_ci;",
                "create table if not exists friendRequest(friendRequestId int primary key auto_increment comment '请求的id',sourceId        int comment '发送信息的人的id',destId          int comment '接收信息的人的id',processed       bool comment '请求是否已经被处理了。') comment '好友请求表' character set utf8mb4 collate utf8mb4_unicode_ci;",
                "create table if not exists groupRequest(groupRequestId int primary key auto_increment comment '请求的id',sourceId       int comment '发送信息的人的id',destId         int comment '接收信息的人的id',processed      bool comment '请求是否已经被处理了。') comment '群组请求表' character set utf8mb4 collate utf8mb4_unicode_ci;"};
        for (int i = 0; i < 8; ++i) {
            mysql_query(conn, sqls[i].c_str());
            assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        }
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return true;
}


MysqlConnectionPool *MysqlConnectionPool::get_mysql_connection_pool_singleton_instance_() {  // 懒汉式构造单例
    if (nullptr == mysql_connection_pool_singleton_instance_) {
        pthread_mutex_lock(&singleton_instance_mutex_);
        if (nullptr == mysql_connection_pool_singleton_instance_) {
            mysql_connection_pool_singleton_instance_ = new MysqlConnectionPool();
        }
        pthread_mutex_unlock(&singleton_instance_mutex_);
    }
    return mysql_connection_pool_singleton_instance_;
}


/**
 * 获取一个空闲连接，如果成功返回true，否则返回false。
 * @param conn
 * @return
 */
bool MysqlConnectionPool::GetConnection(MYSQL **conn) {
    bool get_flag = false;
    pthread_mutex_lock(&mysql_pool_mutex_);
    if (!mysql_pool_.empty()) {
        *conn = mysql_pool_[0];
        mysql_pool_.erase(mysql_pool_.begin(), mysql_pool_.begin() + 1);
        get_flag = true;  // 如果有空闲连接，那么就是true，否则就是false。
        ++used_connection_num_;
    }
    pthread_mutex_unlock(&mysql_pool_mutex_);
    return get_flag;
}

/**
 * 归还一个连接给连接池。
 * @param conn
 * @return
 */
bool MysqlConnectionPool::ReleaseConnection(MYSQL *conn) {
    if (nullptr == conn) {  // 因为是空连接，所以释放失败。
        return false;
    }
    pthread_mutex_lock(&mysql_pool_mutex_);
    mysql_pool_.emplace_back(conn);
    --used_connection_num_;
    pthread_mutex_unlock(&mysql_pool_mutex_);
    return true;
}


/**
 * 登录，查看用户名密码是否正确。
 * @param username
 * @param password
 * @return
 */
bool MysqlConnectionPool::Login(int &user_id, const std::string &username, const std::string &password) {
    bool ret = false;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);
        char *sql = new char[256]{'\0'};
        snprintf(sql, 256, "select * from user where username = '%s' and password = '%s';", username.c_str(),
                 password.c_str());
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        ret = mysql_num_rows(res) > 0;  // 判断查询结果是否存在，存在就代表登录成功。
        while (MYSQL_ROW row = mysql_fetch_row(res)) {
            user_id = atoi(row[0]);
        }
        mysql_free_result(res);  // 及时的释放结果。
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}


/**
 * 注册，查看用户名是否重名，如果没有重名，直接插入，都成功则返回true。
 */
bool MysqlConnectionPool::Register(const std::string &username, const std::string &password) {
    bool ret = false;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[256]{'\0'};
        snprintf(sql, 256, "select * from user where username = '%s';", username.c_str());
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        ret = mysql_num_rows(res) == 0;  // 判断查询结果是否为空，如果不为空，那就说明存在重名。
        mysql_free_result(res);  // 及时的释放结果。
        if (!ret) {
            pthread_mutex_unlock(&mysql_pool_mutex_);
            delete[] sql;
            ReleaseConnection(conn);  // 使用结束请将资源释放掉。
            return false;
        }
        snprintf(sql, 256, "insert into user(username, password) values('%s', '%s');", username.c_str(),
                 password.c_str());
        mysql_query(conn, sql);
        res = mysql_store_result(conn);
        ret = mysql_affected_rows(conn) == 1;
        mysql_free_result(res);  // 及时的释放结果。
        if (!ret) {
            pthread_mutex_unlock(&mysql_pool_mutex_);
            delete[] sql;
            ReleaseConnection(conn);  // 使用结束请将资源释放掉。
            return false;
        }
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}

/**
 * 插入好友请求。
 */
bool MysqlConnectionPool::sendAddFriendRequest(int sourceId, int destId, bool processed) {
    bool ret = false;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[256]{'\0'};
        snprintf(sql, 256, "insert into friendRequest(sourceId, destId, processed) values (%d, %d, %d);", sourceId,
                 destId, processed);
        mysql_query(conn, sql);
        ret = mysql_affected_rows(conn) == 1;
        if (!ret) {
            pthread_mutex_unlock(&mysql_pool_mutex_);
            delete[] sql;
            ReleaseConnection(conn);  // 使用结束请将资源释放掉。
            return false;
        }
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}

/**
 * 直接执行一条给定的sql语句。
 * @param sql
 * @return
 */
bool MysqlConnectionPool::processSql(const char *sql) {
    bool ret = false;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        mysql_query(conn, sql);
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}

/**
 * 找到userId的所有好友。
 * @param userId
 * @return
 */
std::vector<std::pair<int, std::string>> MysqlConnectionPool::findAllFriendsId(int userId) {
    std::vector<std::pair<int, std::string>> ret;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[256]{'\0'};
        snprintf(sql, 256,
                 "select sourceId, username from friendRelation inner join user on sourceId = user.userId  where destId = %d;",
                 userId);
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        while (MYSQL_ROW row = mysql_fetch_row(res)) {  // 取出每一个结果。
            ret.emplace_back(std::make_pair(atoi(row[0]), row[1]));
        }
        mysql_free_result(res);  // 及时的释放结果。
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}

/**
 * 创建群组。
 * @param sourceId
 * @param groupName
 * @return
 */
bool MysqlConnectionPool::createGroup(int sourceId, const std::string &groupName) {
    bool ret = false;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[256]{'\0'};
        snprintf(sql, 256, "select * from `group` where groupName = '%s';", groupName.c_str());
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        ret = mysql_num_rows(res) == 0;  // 判断查询结果是否为空，如果不为空，那就说明存在重名。
        mysql_free_result(res);  // 及时的释放结果。
        if (!ret) {
            pthread_mutex_unlock(&mysql_pool_mutex_);
            delete[] sql;
            ReleaseConnection(conn);  // 使用结束请将资源释放掉。
            return false;
        }
        snprintf(sql, 256, "insert into `group`(groupName, masterId) values ('%s', %d);", groupName.c_str(), sourceId);
        mysql_query(conn, sql);
        res = mysql_store_result(conn);
        ret = mysql_affected_rows(conn) == 1;
        int groupId;
        if (ret) {
            groupId = mysql_insert_id(conn);
        }
        mysql_free_result(res);  // 及时的释放结果。
        if (!ret) {
            pthread_mutex_unlock(&mysql_pool_mutex_);
            delete[] sql;
            ReleaseConnection(conn);  // 使用结束请将资源释放掉。
            return false;
        }
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        snprintf(sql, 256, "insert into groupRelation(sourceId, destId, lastReadTime) values (%d, %d, now());", groupId,
                 sourceId);
        mysql_query(conn, sql);
        res = mysql_store_result(conn);
        ret = mysql_affected_rows(conn) == 1;
        mysql_free_result(res);  // 及时的释放结果。
        if (!ret) {
            pthread_mutex_unlock(&mysql_pool_mutex_);
            delete[] sql;
            ReleaseConnection(conn);  // 使用结束请将资源释放掉。
            return false;
        }
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}


/**
 * 找到userId的所有好友。
 * @param userId
 * @return
 */
std::vector<std::pair<int, std::string>> MysqlConnectionPool::findAllGroups(int userId) {
    std::vector<std::pair<int, std::string>> ret;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[256]{'\0'};
        snprintf(sql, 256,
                 "select groupId, groupName from `group` where groupId in (select sourceId from groupRelation where destId = %d);",
                 userId);
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        while (MYSQL_ROW row = mysql_fetch_row(res)) {  // 取出每一个结果。
            ret.emplace_back(std::make_pair(atoi(row[0]), row[1]));
        }
        mysql_free_result(res);  // 及时的释放结果。
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}


/**
 * 插入群组添加请求。
 */
bool MysqlConnectionPool::sendAddGroupRequest(int sourceId, int destId, bool processed) {
    bool ret = false;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[256]{'\0'};
        snprintf(sql, 256, "insert into groupRequest(sourceId, destId, processed) values (%d, %d, %d);", sourceId,
                 destId, processed);
        mysql_query(conn, sql);
        ret = mysql_affected_rows(conn) == 1;
        if (!ret) {
            pthread_mutex_unlock(&mysql_pool_mutex_);
            delete[] sql;
            ReleaseConnection(conn);  // 使用结束请将资源释放掉。
            return false;
        }
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}

/**
 * 查询群主的id。
 * @param groupId
 * @return
 */
int MysqlConnectionPool::findUserIdByGroupId(int groupId) {
    int ret;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[256]{'\0'};
        snprintf(sql, 256, "select masterId from `group` where groupId = %d;", groupId);
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        while (MYSQL_ROW row = mysql_fetch_row(res)) {  // 取出每一个结果。
            ret = atoi(row[0]);
        }
        mysql_free_result(res);  // 及时的释放结果。
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}

/**
 * 找到userId的所有待处理请求。
 * @param userId
 * @return
 */
std::vector<std::pair<int, std::string>> MysqlConnectionPool::findAllWaitProcessFriendRequest(int userId) {
    std::vector<std::pair<int, std::string>> ret;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[256]{'\0'};
        snprintf(sql, 256,
                 "select user.userId, user.username from user where userId in (select sourceId from friendRequest where processed = 0 and destId = %d);",
                 userId);
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        while (MYSQL_ROW row = mysql_fetch_row(res)) {  // 取出每一个结果。
            ret.emplace_back(std::make_pair(atoi(row[0]), row[1]));
        }
        mysql_free_result(res);  // 及时的释放结果。
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}

/**
 * 找到userId的所有待处理群请求。
 * @param userId
 * @return
 */
std::vector<std::vector<std::string>> MysqlConnectionPool::findAllWaitProcessGroupRequest(int userId) {
    std::vector<std::vector<std::string>> ret;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[512]{'\0'};
        snprintf(sql, 512,
                 "select user.username, res.* from user inner join (select groupRequest.sourceId, groupRequest.destId, `group`.groupName from groupRequest inner join `group` on groupRequest.destId = `group`.groupId where `group`.masterId = %d and groupRequest.processed = 0) res on res.sourceId = user.userId;",
                 userId);
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        while (MYSQL_ROW row = mysql_fetch_row(res)) {  // 取出每一个结果。分别是请求者名字，请求者id,群id，群名字。
            std::vector<std::string> tmp{row[0], row[1], row[2], row[3]};
            ret.emplace_back(tmp);
        }
        mysql_free_result(res);  // 及时的释放结果。
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}

/**
 * 找到(sourceId, destId)所有未读取的内容,但是这里一定是不需要判断时间的，反正是全盘读取。
 * @param sourceId
 * @param destId
 * @return
 */
std::vector<std::vector<std::string>> MysqlConnectionPool::findAllUnreadFriendMessage(int sourceId, int destId) {
    std::vector<std::vector<std::string>> ret;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[1024]{'\0'};
        snprintf(sql, 1024,
                 "select res.friendMessageId, res.sourceId, user.username, res.content, res.sendTime from user inner join (select * from friendMessage where sourceId = %d and destId = %d) res on res.sourceId = user.userId union select res.friendMessageId, res.sourceId, user.username, res.content, res.sendTime from user inner join (select * from friendMessage where sourceId = %d and destId = %d) res on res.sourceId = user.userId order by sendTime;",
                 sourceId, destId, destId, sourceId);
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        while (MYSQL_ROW row = mysql_fetch_row(res)) {  // 取出每一个结果。分别是聊天记录id, 请求者id，请求者name,信息内容，时间。
            std::vector<std::string> tmp{row[0], row[1], row[2], row[3], row[4]};
            ret.emplace_back(tmp);
        }
        mysql_free_result(res);  // 及时的释放结果。
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        snprintf(sql, 1024, "update friendRelation set lastReadTime = now() where sourceId = %d and destId = %d;",
                 sourceId, destId);  // 更新最后的读取时间。
        mysql_query(conn, sql);
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}


/**
 * 找到(sourceId, destId)所有未读取的内容有多少条。
 * @param sourceId
 * @param destId
 * @return
 */
int MysqlConnectionPool::findAllUnreadFriendMessageCount(int sourceId, int destId) {
    int ret = -1;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[1024]{'\0'};
        snprintf(sql, 1024,
                 "select count(*) as num from user inner join (select * from friendMessage where sourceId = %d and destId = %d and sendTime > (select lastReadTime from friendRelation where sourceId = %d and destId = %d)) res on res.sourceId = user.userId;",
                 sourceId, destId, sourceId, destId);
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        while (MYSQL_ROW row = mysql_fetch_row(res)) {  // 取出每一个结果。分别是聊天记录id, 请求者id，请求者name,信息内容，时间。
            ret = atoi(row[0]);
        }
        mysql_free_result(res);  // 及时的释放结果。
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}

/**
 * 找到(sourceIid, destId)所有未读取的群聊天内容。
 * @param sourceId
 * @param destId
 * @return
 */
std::vector<std::vector<std::string>> MysqlConnectionPool::findAllGroupMessage(int sourceId, int destId) {
    std::vector<std::vector<std::string>> ret;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[1024]{'\0'};
        snprintf(sql, 1024,
                 "select res.groupMessageId, res.sourceId, res.innerSourceId, user.username, res.content, res.sendTime from user inner join (select * from groupMessage where sourceId = %d) res on user.userId = res.innerSourceId;",
                 sourceId);
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        while (MYSQL_ROW row = mysql_fetch_row(res)) {  // 取出每一个结果。分别是聊天记录id, 请求者id，请求者name,信息内容，时间。
            std::vector<std::string> tmp{row[0], row[1], row[2], row[3], row[4], row[5]};
            ret.emplace_back(tmp);
        }
        mysql_free_result(res);  // 及时的释放结果。
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        snprintf(sql, 1024, "update groupRelation set lastReadTime = now() where sourceId = %d and destId = %d;",
                 sourceId, destId);  // 更新最后的读取时间。
        mysql_query(conn, sql);
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}


/**
 * 找到(sourceId, destId)所有未读取的内容有多少条。
 * @param sourceId
 * @param destId
 * @return
 */
int MysqlConnectionPool::findAllUnreadGroupMessageCount(int sourceId, int destId) {
    int ret = -1;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[1024]{'\0'};
        snprintf(sql, 1024,
                 "select count(*) from user inner join (select * from groupMessage where sourceId = %d and sendTime > (select lastReadTime from groupRelation where sourceId = %d and destId = %d)) res on user.userId = res.innerSourceId;",
                 sourceId, sourceId, destId);
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        while (MYSQL_ROW row = mysql_fetch_row(res)) {  // 取出每一个结果。分别是聊天记录id, 请求者id，请求者name,信息内容，时间。
            ret = atoi(row[0]);
        }
        mysql_free_result(res);  // 及时的释放结果。
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}


/**
 * 找到对应群聊中所有的用户id。
 * @param groupId
 * @return
 */
std::vector<int> MysqlConnectionPool::findAllMemberInGroup(int groupId) {
    std::vector<int> ret;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[1024]{'\0'};
        snprintf(sql, 1024, "select destId from groupRelation where sourceId = %d;", groupId);
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        while (MYSQL_ROW row = mysql_fetch_row(res)) {
            ret.emplace_back(atoi(row[0]));
        }
        mysql_free_result(res);  // 及时的释放结果。
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}


/**
 * 查询群的名字。
 * @param groupId
 * @return
 */
std::string MysqlConnectionPool::findGroupNameByGroupId(int groupId) {
    std::string ret;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[256]{'\0'};
        snprintf(sql, 256, "select groupName from `group` where groupId = %d;", groupId);
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        while (MYSQL_ROW row = mysql_fetch_row(res)) {  // 取出每一个结果。
            ret = row[0];
        }
        mysql_free_result(res);  // 及时的释放结果。
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}



/**
 * 查询用户的名字。
 * @param groupId
 * @return
 */
std::string MysqlConnectionPool::findUserNameByUserId(int userId) {
    std::string ret;
    MYSQL *conn = nullptr;
    if (GetConnection(&conn)) {
        pthread_mutex_lock(&mysql_pool_mutex_);  // 操作连接池一定要锁定，不然就会出现线程不安全的问题。
        char *sql = new char[256]{'\0'};
        snprintf(sql, 256, "select username from user where userId = %d;", userId);
        mysql_query(conn, sql);
        MYSQL_RES *res = mysql_store_result(conn);
        while (MYSQL_ROW row = mysql_fetch_row(res)) {  // 取出每一个结果。
            ret = row[0];
        }
        mysql_free_result(res);  // 及时的释放结果。
        assert(mysql_errno(conn) == 0);  // 这时候是没有发生错误的
        pthread_mutex_unlock(&mysql_pool_mutex_);
        delete[] sql;
    }
    ReleaseConnection(conn);  // 使用结束请将资源释放掉。
    return ret;
}
