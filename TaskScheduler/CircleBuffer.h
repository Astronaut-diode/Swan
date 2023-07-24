//
// Created by diode on 23-7-23.
// 唤醒缓冲区的数据结构。

#ifndef SWAN_CIRCLEBUFFER_H
#define SWAN_CIRCLEBUFFER_H

#include <iostream>
#include <vector>

template<int LEN, typename T>
class CircleBuffer {
public:  // 用于写typedef或者静态常量等。

private:  // 变量区域

public:
    std::vector<T> data_;  // 环形缓冲区中保存的内容。

private:  // 函数区域

public:
    CircleBuffer<LEN, T>() {
        data_.resize(LEN);
    }

    ~CircleBuffer<LEN, T>() {

    }

    void push_back(T tmp) {  // 将tmp插入到data_中，而且要将首元素弹出。
        data_.erase(data_.begin());
        data_.push_back(tmp);
    }

    int size() {
        return LEN;
    }

    T& operator[](const int& i) {
        return data_[i];
    };

    const T& operator[](const int& i) const {
        return data_[i];
    };
};

#endif //SWAN_CIRCLEBUFFER_H
