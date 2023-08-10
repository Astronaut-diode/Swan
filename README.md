# Swan
可以私聊以及群聊的即时通讯系统。

## 使用方法
1.创建Swan数据库。
create database Swan character set utf8mb4 collate utf8mb4_unicode_ci;

2.修改mysql的配置[my.cnf]，并记得重启。
[mysqld]
sql_mode='STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION'

3.修改Config.cpp中的Config构造函数，其中包含了mysql以及redis的连接参数。

4.main.cpp:9中的true代表日志输出到控制台，false代表输出到日志文件。

5.如果有nginx服务，可以参考nginx.conf中的配置，并启动对应sbin下的nginx，如[/usr/local/nginx/sbin/nginx];后面访问localhost即可。

6.使用makefile编译,命令./Swan [服务器的ip地址] [服务器监听的端口号] 即可启动。

如果是使用了阿里云，那么nginx中负载均衡的地址是对应的私地址，启动的ip地址使用对应的私地址，但是连接websocket的ip地址需要是给定的公网地址(更改Response.cpp:35)。
``` c++
if (target) {
    memset(target, ' ', 21);
//        std::string tmp =
//                Config::get_singleton_()->server_ip_ + +":" + std::to_string(Config::get_singleton_()->server_port_);
    std::string tmp = std::string("公网地址:") + std::to_string(Config::get_singleton_()->server_port_);
    strncpy(target, tmp.c_str(), tmp.size());
}
```
