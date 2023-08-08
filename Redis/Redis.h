//
// Created by diode on 23-7-5.
// redis的连接池头文件。

#ifndef DISKSERVER_REDIS_H
#define DISKSERVER_REDIS_H

#include <pthread.h>
#include <string>
#include <hiredis/hiredis.h>
#include <cassert>
#include <vector>
#include <semaphore.h>
#include "../Config/Config.h"
#include "../Utils/Utils.h"

class Redis {
private:
    static Redis *singleton_;
    static pthread_mutex_t singleton_mutex_;

    std::string host_;
    std::string password_;
    unsigned port_;

    unsigned pool_max_count_;  // pool的最大容量。
    pthread_mutex_t pool_mutex_;
    std::vector<redisContext *> pool_;  // 连接池。
    pthread_mutex_t connection_mutex_;
public:

private:
    Redis();

    ~Redis();

public:
    static Redis *get_singleton_();

    bool GetConnection(redisContext **conn);  // 获取一个redis连接，成功则返回true。

    bool ReleaseConnection(redisContext *conn);  // 释放一个连接，成功返回true。

    std::string SessionExists(const int &userId);  // 判断目标的userId是否有对应的session记录。

    void removeSession(const int &userId);  // 关闭用户的连接

    bool publish(const std::string &channelName, int sourceId, int destId);  // 向指定的频道推送消息。
};

#endif
