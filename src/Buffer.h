#pragma once
#include <string>
#include <sys/types.h>

class Buffer{
private:
    //TODO 如果消息很长可以优化这里的实现！！！
    std::string buf;

public:
    Buffer();
    ~Buffer();
    void append(const char* _str,int _size);
    ssize_t size();
    const char* c_str();
    void clear();
    void getline();
};