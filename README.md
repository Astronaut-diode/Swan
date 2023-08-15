# Swan

## Introduce

本项目为C++11编写的集群聊天服务器，建立在WebSocket协议之上，支持用户注册、用户登陆、添加好友、创建群组、添加群组、私聊、群聊、离线消息等功能。

## Technical points

- 在解析HTTP请求的基础上，升级为使用**WebSocket**协议，以减少客户端对服务器请求次数，主动推送信息； 
- 实现muduo中的**one loop per thread**模型，利于高效合理的分发事件到对应线程执行对应任务； 
- 使用**双缓冲**及**条件变量**实现高效的异步日志系统、避免数据落盘时阻塞服务； 
- 使用基于**红黑树**的**时间轮**管理超时连接，高效管理非活跃任务； 
- 使用**智能指针**管理内存，减少内存泄漏风险； 
- 使用Nginx的TCP**负载均衡**功能，将客户端连接分发到多个服务器上，缓解服务器压力； 
- 基于Redis的**发布-订阅**功能，解决跨服务器之间的通信问题；


## Environment

- OS: Ubuntu 18.04
- MySQL: 5.7.42
- Redis: 4.0.9
- Compiler: g++ 7.5
- 浏览器测试：Windows、Linux下使用Chrome、Edge、FireFox均可。


## Usage

1. 修改mysql的配置[my.cnf]，向其中加入一行，并记得重启。

   ``` 
   [mysqld]
   sql_mode='STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION'
   ```

2. 创建Swan数据库。

   ``` sql
   create database Swan character set utf8mb4 collate utf8mb4_unicode_ci;
   ```

3. 修改Config.cpp中的Config构造函数，其中包含了mysql以及redis的连接参数。

4. 如果有nginx服务，可以参考nginx.conf中的配置，并启动对应sbin下的nginx，如[/usr/local/nginx/sbin/nginx];后面访问localhost即可。

5. 如果是使用了阿里云，那么nginx中负载均衡的地址是对应的私地址，启动的ip地址使用对应的私地址，但是连接websocket的ip地址需要是给定的公网地址(更改Response.cpp:35)。

   ``` c++
   if (target) {
       memset(target, ' ', 21);
   //  std::string tmp = Config::get_singleton_()->server_ip_ + ":" + std::to_string(Config::get_singleton_()->server_port_);
       std::string tmp = std::string("公网地址:") + std::to_string(Config::get_singleton_()->server_port_);
       strncpy(target, tmp.c_str(), tmp.size());
   }
   ```

6. ``` shell
   make # 进行编译
   ```

7. ``` shell
   ./Swan [监听的IP地址] [监听的端口号] # 
   ```

8. 浏览器端访问**IP地址:端口号**


## 致谢

[TinyWebServer](https://github.com/qinguoyi/TinyWebServer)

[muduo](https://github.com/chenshuo/muduo)

