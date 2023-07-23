//
// Created by diode on 23-7-22.
// 一些常用的工具方法,通常都是静态方法可以直接使用。

#ifndef SWAN_UTILS_H
#define SWAN_UTILS_H

#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <iostream>

class Utils {
public:
    /**
     * 根据dirPath一次性创建多级目录，而且内容的长度要大于1，不能以分割符结尾。
     * @param dirPath
     * @param size
     */
    static void mkdir(char *dirPath, int size) {
        assert(size > 1);  // 长度要满足大于1，只有大于1才能够避免根目录。
        assert(dirPath[size - 1] != '/');  // 最后一个元素不能是/。
        for(int i = 1; i < size; ++i) {  // 判断父级目录是否存在。
            if(dirPath[i] == '/') {
                dirPath[i] = '\0';
                if(access(dirPath, 0)) {
                    ::mkdir(dirPath, 0755);
                }
                dirPath[i] = '/';
            }
        }
        if(access(dirPath, 0)) {  // 创建最后一级目录。
            ::mkdir(dirPath, 0755);
        }
    }


};

#endif //SWAN_UTILS_H
