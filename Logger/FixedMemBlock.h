//
// Created by diode on 23-7-22.
// 是一个固定长度的数据结构，底层只封装了一个字符数组以及字符指针，用于异步日志系统中的双缓冲内存块。

#ifndef SWAN_FIXEDMEMBLOCK_H
#define SWAN_FIXEDMEMBLOCK_H

#include <cstring>

template<int LEN>
class FixedMemBlock {
public:  // 用于写typedef或者静态常量等。

private:  // 变量区域
    char memCache_[LEN];  // 用于记录内容的区域。
    char *cur_;  // 当前已经写到什么位置了。
public:

private:  // 函数区域
    /**
     * 还剩多少内容可以写。
     * @return
     */
    int writeAble() {
        return LEN - static_cast<int>(cur_ - memCache_);
    }

public:
    FixedMemBlock() : cur_(memCache_) {

    }

    ~FixedMemBlock() {

    }

    /**
     * 往memCache_中追加内容。
     * @return
     */
    bool append(const char *data, int size) {
        if (size > writeAble()) {
            return false;
        }
        memcpy(cur_, data, size);
        cur_ = cur_ + size;
        return true;
    }

    /**
     * 将内容修改为空，返回原始状态。
     */
    void reset() {
        cur_ = memCache_;
        memset(cur_, '\0', LEN);
    }

    /**
     * 当前已经写了多少内容。
     * @return
     */
    int size() {
        return static_cast<int>(cur_ - memCache_);
    }

    /**
     * 返回对应的内容。
     * @return
     */
    char *data() {
        return memCache_;
    }
};

#endif //SWAN_FIXEDMEMBLOCK_H
