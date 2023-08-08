//
// Created by diode on 23-7-22.
// 一些常用的工具方法,通常都是静态方法可以直接使用。

#ifndef SWAN_UTILS_H
#define SWAN_UTILS_H

#include <fcntl.h>
#include <csignal>
#include <cassert>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <cstring>
#include <dirent.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <random>

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

    static int setNonBlocking(int fd) {
        int old_version = fcntl(fd, F_GETFL);
        int new_option = old_version | O_NONBLOCK;
        fcntl(fd, F_SETFL, new_option);
        return old_version;
    }

    /**
 * 生成固定位数的sesison
 * @param length
 * @return
 */
    static std::string GenerationSession(int length) {
        static std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<int> distribution(0, charset.length() - 1);
        std::string result;
        for (int i = 0; i < length; ++i) {
            result += charset[distribution(generator)];
        }
        return result;
    }

};

#endif //SWAN_UTILS_H
