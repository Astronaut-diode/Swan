//
// Created by diode on 23-7-22.
// 异步日志类的实现文件，负责管理记录异步日志的线程，并实现双缓冲方法。

#include "AsyncLog.h"

#include <memory>

/**
 * 异步子线程需要执行的工作。
 */
void AsyncLog::ThreadCallBack() {
    BufferPtr backCurrent_ = std::make_unique<Buffer>();  // 初始化创建这两块区域。
    BufferPtr backNext_ = std::make_unique<Buffer>();  // 后端的两块内存块。
    backCurrent_->reset();
    backNext_->reset();
    BufferVec backBuffers_;
    backBuffers_.reserve(16);  // 预留为16块内容。
    while (true) {  // 开始执行后端的监听任务，不断的交换前后端数据，并进行写入工作。
        assert(backCurrent_ && backCurrent_->size() == 0);  // 指针不能为空，而且内容是没有写过的。
        assert(backNext_ && backNext_->size() == 0);
        assert(backBuffers_.empty());  // 保持后端的内容在新一轮的时候一定是清空的。
        pthread_mutex_lock(&mutex_);
        if (buffers_.empty()) {
            struct timeval now;  // 获取当前的时间。
            gettimeofday(&now, nullptr);
            next_.tv_sec = now.tv_sec + kMaxInterval_;
            next_.tv_nsec = 0;  // 更新超时时间。
            ::pthread_cond_timedwait(&cond_, &mutex_, &next_);  // 最多等待三秒钟，就开始交换前后端内容。
        }
        buffers_.push_back(std::move(currentPtr_));
        backBuffers_.swap(buffers_);  // 交换两者内容。
        currentPtr_ = std::move(backCurrent_);
        if (!nextPtr_) {  // 如果nextPtr已经交给了currentPtr_，那么是需要给next一块新的内存块的。
            nextPtr_ = std::move(backNext_);
        }
        pthread_mutex_unlock(&mutex_);

        assert(!backBuffers_.empty());
        if (backBuffers_.size() > 25) {  // 超标太多了，说明已经处理不过来了，需要删除内容。
            backBuffers_.erase(backBuffers_.begin() + 2, backBuffers_.end());
        } else {
            for (int i = 0; i < backBuffers_.size(); ++i) {
                logFile_->append(backBuffers_[i]->data(), backBuffers_[i]->size());  // 将内容写入目标。
            }
        }
        if (backBuffers_.size() > 2) {
            backBuffers_.resize(2);  // 只留下两个部分。
        }
        if (!backCurrent_) {  // 如果已经换给了前端,那就需要重新赋值。同时要把内容重新置空。
            assert(!backBuffers_.empty());
            backCurrent_ = std::move(backBuffers_.back());
            backBuffers_.pop_back();
            backCurrent_->reset();
        }
        if (!backNext_) {
            assert(!backBuffers_.empty());
            backNext_ = std::move(backBuffers_.back());
            backBuffers_.pop_back();
            backNext_->reset();
        }
        backBuffers_.clear();
    }
}

AsyncLog::AsyncLog(bool outputTerminal) : outputTerminal_(outputTerminal) {
    currentPtr_ = std::make_unique<Buffer>();  // 初始化创建这两块区域。
    nextPtr_ = std::make_unique<Buffer>();
    buffers_.reserve(16);  // 预留出16块的区域。
    if (!outputTerminal_) {  // 说明要写到文件中，那就判断目标路径是否已经创建成功。
        memset(logDirPath_, '\0', kLogFilePathLength);  // 初始化日志目录路径。
        assert(getcwd(logDirPath_, kLogFilePathLength));  // 获取当前进程执行的时候使用的目录路径。
        strcat(logDirPath_, kLogDirPath);  // 保存日志的路径。
        Utils::mkdir(logDirPath_, strlen(logDirPath_));  // 创建该路径，并且可一次性创建多级目录。
    }
    thread_ = std::make_unique<Thread>(kLogThreadName, std::bind(&AsyncLog::ThreadCallBack, this));  // 创建对应的线程用例。
    thread_->createThread();  // 创建并启动子线程，让其执行给定的任务。
    pthread_mutex_init(&mutex_, nullptr);  // 初始化两把锁。
    pthread_cond_init(&cond_, nullptr);
    logFile_ = std::make_unique<LogFile>(outputTerminal_, logDirPath_);  // 创建对应的日志文件的对象。
}

AsyncLog::~AsyncLog() {
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
}

/**
 * 往前端的内容中增加内容。
 * @param data
 * @param size
 */
void AsyncLog::appendToCurrent(const char *data, int size) {
    pthread_mutex_lock(&mutex_);
    bool ret = currentPtr_->append(data, size);
    if (!ret) {  // 添加失败，就代表内存已经满了，需要换内存块了。
        buffers_.push_back(std::move(currentPtr_));
        if (nextPtr_) {  // 判断nextPtr_是否存在，如果在就可以直接给currentPtr_了。
            currentPtr_ = std::move(nextPtr_);
        } else {
            currentPtr_ = std::make_unique<Buffer>();
            currentPtr_->reset();
        }
        assert(currentPtr_->append(data, size));  // 重新插入，而且这一次的结果一定是需要是true的。
        pthread_cond_signal(&cond_);  // 条件已经满足了，直接唤醒后端进行处理。
    }
    pthread_mutex_unlock(&mutex_);
}