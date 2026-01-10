#include "Channel.h"
#include "EventLoop.h"
#include <cstdint>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>

//要区别EventLoop和对应的loop函数，这里是对象
Channel::Channel(EventLoop* _loop,int _fd):loop(_loop),fd(_fd),
                                events(0),ready(0),inEpoll(false){

}
Channel::~Channel(){
    if(fd != -1){
        close(fd);
        fd = -1;
    }
}

void Channel::handleEvent(){
    if(ready & (EPOLLIN | EPOLLPRI)){
        if(readCallback){  // 检查回调函数是否为空，避免空指针调用
            if(useThreadPool)       
                loop->addThread(readCallback);
            else
                readCallback();
        }
    }
    if(ready & (EPOLLOUT)){
        if(writeCallback){  
            if(useThreadPool)       
                loop->addThread(writeCallback);
            else
                writeCallback();
        }
    }
}

void Channel::enableRead(){
    events |= EPOLLIN | EPOLLPRI;
    loop -> updateChannel(this);
}

void Channel::useET(){
    events |= EPOLLET;
    loop->updateChannel(this);
}

int Channel::getFd(){
    return fd;
}
uint32_t Channel::getEvents(){
    return events;
}
uint32_t Channel::getReady(){
    return ready;
}

bool Channel::getInEpoll(){
    return inEpoll;
}

void Channel::setInEpoll(bool _in){
    inEpoll = _in;
}

void Channel::setReady(uint32_t _ev){
    ready = _ev;
}

void Channel::setReadCallback(std::function<void ()> _cb){
    readCallback = _cb;
}
void Channel::setUseThreadPool(bool use){
    useThreadPool = use;
}