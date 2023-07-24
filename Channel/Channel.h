//
// Created by diode on 23-7-23.
//

#ifndef SWAN_CHANNEL_H
#define SWAN_CHANNEL_H

#include <functional>
#include <sys/epoll.h>
#include <sys/time.h>


typedef std::function<void()> ReadFunctionCallBack;
typedef std::function<void()> WriteFunctionCallBack;
typedef std::function<void()> CloseFunctionCallBack;
typedef std::function<void()> ErrorFunctionCallBack;

class Channel {
public:  // 用于写typedef或者静态常量等。

private:  // 变量区域
    int fd_;  // 使用的文件描述符。
    int events_;  // 关心的事件。
    int revents_;  // 真实发生的事件。
    int useByPoller_;  // 在poller中的状态。
    ReadFunctionCallBack readFunctionCallBack_;  // 四种函数的回调函数。
    WriteFunctionCallBack writeFunctionCallBack_;
    CloseFunctionCallBack closeFunctionCallBack_;
    ErrorFunctionCallBack errorFunctionCallBack_;

public:

private:  // 函数区域

public:
    void setReadFunctionCallBack(ReadFunctionCallBack readFunctionCallBack);

    void setWriteFunctionCallBack(WriteFunctionCallBack writeFunctionCallBack);

    void setCloseFunctionCallBack(CloseFunctionCallBack closeFunctionCallBack);

    void setErrorFunctionCallBack(ErrorFunctionCallBack errorFunctionCallBack);

    Channel(int fd);

    ~Channel();

    void handleEvent();  // 接收到信号的时候开始处理。

    void enableRead();  // 设置channel对read感兴趣。

    void enableWrite();  // 设置channel对write感兴趣。

    void update();  // 更新epoll监听事件。

    int events();

    int fd();

    void setRevents(int event);  // 设置发生的事情。

    void disableRead();

    void disableWrite();
};

#endif //SWAN_CHANNEL_H
