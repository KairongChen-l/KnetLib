#include "knetlib/AsyncLogging.h"
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <algorithm>

AsyncLogging::AsyncLogging(const std::string& basename, off_t rollSize, int flushInterval)
        : basename_(basename),
          rollSize_(rollSize),
          flushInterval_(flushInterval),
          running_(false),
          currentBuffer_(new Buffer),
          nextBuffer_(new Buffer)
{
    currentBuffer_->reserve(kBufferSize);
    nextBuffer_->reserve(kBufferSize);
    currentBuffer_->resize(0);
    nextBuffer_->resize(0);
}

AsyncLogging::~AsyncLogging() {
    if (running_) {
        stop();
    }
}

void AsyncLogging::start() {
    running_ = true;
    thread_ = std::thread(&AsyncLogging::threadFunc, this);
}

void AsyncLogging::stop() {
    running_ = false;
    cond_.notify_one();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void AsyncLogging::append(const char* logline, int len) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 如果当前缓冲区有足够空间，直接追加
    if (currentBuffer_->size() + len < static_cast<size_t>(kBufferSize)) {
        currentBuffer_->insert(currentBuffer_->end(), logline, logline + len);
    } else {
        // 当前缓冲区满了，移动到待写入队列
        buffers_.push_back(currentBuffer_);
        currentBuffer_.reset();
        
        // 使用备用缓冲区
        if (nextBuffer_) {
            currentBuffer_ = nextBuffer_;
            nextBuffer_.reset();
        } else {
            // 备用缓冲区也被使用了，创建新的
            currentBuffer_.reset(new Buffer);
            currentBuffer_->reserve(kBufferSize);
        }
        
        currentBuffer_->insert(currentBuffer_->end(), logline, logline + len);
        cond_.notify_one();  // 唤醒后台线程
    }
}

void AsyncLogging::threadFunc() {
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->reserve(kBufferSize);
    newBuffer2->reserve(kBufferSize);
    
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);
    
    std::string logFile = basename_;
    if (logFile.find(".log") == std::string::npos) {
        logFile += ".log";
    }
    
    while (running_) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            // 等待缓冲区有数据或超时
            if (buffers_.empty() && (!currentBuffer_ || currentBuffer_->size() == 0)) {
                cond_.wait_for(lock, std::chrono::seconds(flushInterval_));
            }
            
            // 将当前缓冲区也加入待写入队列
            if (currentBuffer_ && currentBuffer_->size() > 0) {
                buffers_.push_back(currentBuffer_);
                currentBuffer_.reset();
            }
            
            // 使用备用缓冲区或创建新的
            if (!newBuffer1) {
                if (!buffers_.empty()) {
                    newBuffer1 = buffers_.back();
                    buffers_.pop_back();
                    newBuffer1->resize(0);
                } else {
                    newBuffer1.reset(new Buffer);
                    newBuffer1->reserve(kBufferSize);
                }
            }
            currentBuffer_ = newBuffer1;
            newBuffer1.reset();
            
            // 交换待写入队列
            buffersToWrite.swap(buffers_);
            
            // 如果备用缓冲区也被使用了，创建新的
            if (!nextBuffer_) {
                if (!newBuffer2) {
                    newBuffer2.reset(new Buffer);
                    newBuffer2->reserve(kBufferSize);
                }
                nextBuffer_ = newBuffer2;
                newBuffer2.reset();
            }
        }
        
        if (buffersToWrite.empty()) {
            continue;
        }
        
        // 如果缓冲区太多，丢弃一些（避免日志堆积）
        if (buffersToWrite.size() > 25) {
            char buf[256];
            snprintf(buf, sizeof(buf), "Dropped log messages at %s, %zd larger buffers\n",
                     basename_.c_str(), buffersToWrite.size() - 2);
            fputs(buf, stderr);
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }
        
        // 写入文件
        for (const auto& buffer : buffersToWrite) {
            // 检查文件大小，需要时滚动
            struct stat st;
            if (stat(logFile.c_str(), &st) == 0) {
                if (st.st_size >= rollSize_) {
                    rollFile();
                    logFile = basename_;
                    if (logFile.find(".log") == std::string::npos) {
                        logFile += ".log";
                    }
                }
            }
            
            // 写入文件
            FILE* fp = fopen(logFile.c_str(), "a");
            if (fp) {
                fwrite(buffer->data(), 1, buffer->size(), fp);
                fclose(fp);
            }
        }
        
        // 清理已写入的缓冲区
        if (buffersToWrite.size() > 2) {
            buffersToWrite.resize(2);
        }
        
        // 重新使用缓冲区
        if (!newBuffer1) {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->resize(0);
        }
        
        if (!newBuffer2) {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer2->resize(0);
        }
        
        buffersToWrite.clear();
    }
    
    // 最后刷新剩余的缓冲区
    std::unique_lock<std::mutex> lock(mutex_);
    if (currentBuffer_ && currentBuffer_->size() > 0) {
        buffers_.push_back(currentBuffer_);
        currentBuffer_.reset();
    }
    if (!buffers_.empty()) {
        buffersToWrite.swap(buffers_);
    }
    lock.unlock();
    
    // 写入剩余数据（使用已有的 logFile 变量）
    FILE* fp = fopen(logFile.c_str(), "a");
    if (fp) {
        for (const auto& buffer : buffersToWrite) {
            fwrite(buffer->data(), 1, buffer->size(), fp);
        }
        fclose(fp);
    }
}

void AsyncLogging::rollFile() {
    time_t now = time(nullptr);
    
    // 如果 basename_ 已经是完整路径，直接使用；否则添加 .log
    std::string filename = basename_;
    if (filename.find(".log") == std::string::npos) {
        filename += ".log";
    }
    
    // 生成带时间戳的文件名
    char timebuf[32];
    struct tm tm;
    gmtime_r(&now, &tm);
    strftime(timebuf, sizeof(timebuf), ".%Y%m%d-%H%M%S", &tm);
    
    // 在 .log 之前插入时间戳
    size_t pos = filename.find(".log");
    if (pos != std::string::npos) {
        filename.insert(pos, timebuf);
    } else {
        filename += timebuf;
        filename += ".log";
    }
    
    // 如果文件已存在，添加序号
    int count = 0;
    std::string finalFilename = filename;
    while (access(finalFilename.c_str(), F_OK) == 0) {
        char buf[512];
        size_t dotPos = filename.find_last_of('.');
        if (dotPos != std::string::npos) {
            std::string base = filename.substr(0, dotPos);
            std::string ext = filename.substr(dotPos);
            snprintf(buf, sizeof(buf), "%s.%d%s", base.c_str(), ++count, ext.c_str());
        } else {
            snprintf(buf, sizeof(buf), "%s.%d", filename.c_str(), ++count);
        }
        finalFilename = buf;
    }
    
    // 重命名当前文件
    std::string currentFile = basename_;
    if (currentFile.find(".log") == std::string::npos) {
        currentFile += ".log";
    }
    if (access(currentFile.c_str(), F_OK) == 0) {
        rename(currentFile.c_str(), finalFilename.c_str());
    }
}

