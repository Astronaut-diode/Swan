//
// Created by diode on 23-6-29.
// 系统的配置类，每个模块的配置都由Config的懒汉式单例对象提供。
//

#ifndef FILEWEBSERVER_CONFIG_H
#define FILEWEBSERVER_CONFIG_H
#include <pthread.h>
#include <string>
#include <cstring>

class Config {
private:
    static Config * singleton_;
    static pthread_mutex_t singleton_mutex_;

public:

private:
    Config();

    ~Config();

public:
    static Config *get_singleton_();  // 获取单例模型的静态函数。

    std::string mysql_host_;  // 数据库连接五要素。
    std::string mysql_port_;
    std::string mysql_username_;
    std::string mysql_password_;
    std::string mysql_database_name_;
    unsigned mysql_connection_max_num_;  // 数据库连接的最大连接数。

    std::string redis_host_;
    unsigned redis_port_;
    std::string redis_password_;
    unsigned redis_pool_max_count_;
    unsigned redis_generator_session_length_;
};
#endif