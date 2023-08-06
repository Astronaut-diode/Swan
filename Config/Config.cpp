//
// Created by diode on 23-6-29.
// 系统的配置类，每个模块的配置都由Config的懒汉式单例对象提供。
//

#include "Config.h"

Config *Config::singleton_ = nullptr;
pthread_mutex_t Config::singleton_mutex_ = PTHREAD_MUTEX_INITIALIZER;  // 默认的初始化参数，即便没有调用初始化函数也可以使用了。

Config::Config() {  // 用于赋值各种参数，别的模块可以通过单例模型直接拿到内容。
    mysql_host_ = "127.0.0.1";
    mysql_port_ = "3306";
    mysql_username_ = "root";
    mysql_password_ = "";
    mysql_database_name_ = "Swan";
    mysql_connection_max_num_ = 8;
    redis_host_ = "localhost";
    redis_port_ = 6379;
    redis_password_ = "123456";
    redis_pool_max_count_ = 8;
    redis_generator_session_length_ = 16;
}

Config::~Config() {

}

/**
 * 懒汉式加载单例模型。
 * @return
 */
Config *Config::get_singleton_() {
    if (nullptr == singleton_) {
        pthread_mutex_lock(&singleton_mutex_);
        if (nullptr == singleton_) {
            singleton_ = new Config();
        }
        pthread_mutex_unlock(&singleton_mutex_);
    }
    return singleton_;
}

