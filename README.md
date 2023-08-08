# Swan
可以私聊以及群聊的即时通讯系统。

## 使用方法
1.create database Swan character set utf8mb4 collate utf8mb4_unicode_ci;

2.设置Config.cpp中的Config构造函数，其中包含了mysql以及redis的连接参数。

3.设置Acceptor.h:24行的服务器端监听端口。

4.修改/Resource/Html/login.html中30行的websocket连接地址为正确的服务器连接地址。

5.main.cpp:9中的true代表日志输出到控制台，false代表输出到日志文件。

6.使用makefile编译运行即可。
