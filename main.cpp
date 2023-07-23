#include <iostream>
#include "Logger/FixedMemBlock.h"
#include "Logger/AsyncLog.h"
#include "Logger/LogStream.h"
#include <fstream>

AsyncLog asyncLog(false);  // 定义为全局的变量,方便LogStream析构的时候，将内容写入到异步线程中。

int main() {
    int index = 0;
    while (true) {
        LOG << "信息" << index++ << "新的信息\n";  // 现在可以通过LOG直接进行异步日志的写入。
        sleep(1);
    }
    return 0;
}