# Swan
可以私聊以及群聊的即时通讯系统。

## 使用方法
1.create database Swan character set utf8mb4 collate utf8mb4_unicode_ci;

2.设置Config.cpp中的Config构造函数，其中包含了mysql以及redis的连接参数。

3.main.cpp:9中的true代表日志输出到控制台，false代表输出到日志文件。

4.如果有nginx服务，可以参考nginx.conf中的配置，并启动对应sbin下的nginx，如[/usr/local/nginx/sbin/nginx];后面访问localhost即可。

5.使用makefile编译,命令./Swan [服务器的ip地址] [服务器监听的端口号] 即可启动。

