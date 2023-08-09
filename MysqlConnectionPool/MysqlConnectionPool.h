//
//  Created by diode on 23-6-25.
//  数据库连接池
//

#ifndef CHATROOM_MYSQLCONNECTIONPOOL_H
#define CHATROOM_MYSQLCONNECTIONPOOL_H

#include <iostream>
#include <mysql/mysql.h>
#include <pthread.h>
#include <string>
#include <cstring>
#include "../Config/Config.h"
#include <vector>
#include <assert.h>

class MysqlConnectionPool {
private:
    std::string host_;  // 数据库连接五要素
    std::string port_;
    std::string username_;
    std::string password_;
    std::string database_name_;
    unsigned connection_max_num_;  // 连接池中最大的连接数量。
    std::vector<MYSQL *> mysql_pool_;  // 连接池
    pthread_mutex_t mysql_pool_mutex_;  // 操作连接池的互斥锁。
    unsigned used_connection_num_;  // 正在使用的连接数量。

    static MysqlConnectionPool *mysql_connection_pool_singleton_instance_;  // 数据库连接池的单例对象以及对应的静态互斥锁。

    static pthread_mutex_t singleton_instance_mutex_;
public:

private:
    MysqlConnectionPool();

    ~MysqlConnectionPool();

    bool InitTable();  // 初始化数据库的表。
public:
    static MysqlConnectionPool *get_mysql_connection_pool_singleton_instance_();

    bool GetConnection(MYSQL **conn);  // 随机获取一个空闲状态的数据库连接，从连接池中取走。

    bool ReleaseConnection(MYSQL *conn);  // 释放一个连接，归还到连接池中。

    bool Login(int &user_id, const std::string &username, const std::string &password);  // 登录，查看用户名密码是否正确。

    bool Register(const std::string &username, const std::string &password);  // 注册，查看用户名是否重名，如果没有重名，直接插入，都成功则返回true。

    bool sendAddFriendRequest(int sourceId, int destId, bool processed);  // 插入好友请求。

    bool processSql(const char *sql);  // 直接执行一条给定的sql语句。

    std::vector<std::pair<int, std::string>> findAllFriendsId(int userId);  // 找到userId的所有好友。

    bool createGroup(int sourceId, const std::string &groupName);  // 创建群组。

    std::vector<std::pair<int, std::string>> findAllGroups(int userId);  // 找到userId的所有群组。

    bool sendAddGroupRequest(int sourceId, int destId, bool processed);  // 插入群组添加请求。

    int findUserIdByGroupId(int groupId);  // 查询群主ID。

    std::vector<std::pair<int, std::string>> findAllWaitProcessFriendRequest(int userId);  // 找到userId的所有待处理好友请求。

    std::vector<std::vector<std::string>> findAllWaitProcessGroupRequest(int userId);  // 找到userId的所有待处理群请求。

    std::vector<std::vector<std::string>> findAllUnreadFriendMessage(int sourceId, int destId);  // 找到(sourceId, destId)所有未读取的内容。

    int findAllUnreadFriendMessageCount(int sourceId, int destId);  // 查询(sourceId, destId)有多少条未读取的内容，只要数量。

    std::vector<std::vector<std::string>> findAllGroupMessage(int sourceId, int destId);  // 找到(sourceIid, destId)所有未读取的群聊天内容。

    int findAllUnreadGroupMessageCount(int sourceId, int destId);  // 查询(sourceId, destId)有多少条未读取的内容，只要数量。

    std::vector<int> findAllMemberInGroup(int groupId);  // 找到对应群聊中所有的用户id。。

    std::string findGroupNameByGroupId(int groupId);  // 查询群组的名字。

    std::string findUserNameByUserId(int userId);  // 查询用户的名字。
};

#endif
