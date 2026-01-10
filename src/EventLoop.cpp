#include "EventLoop.h"
#include "Epoll.h"
#include "Channel.h"
#include <vector>


EventLoop::EventLoop():ep(nullptr),quit(false){
    ep = new Epoll();
    threadPool  = new ThreadPool(); 
}

EventLoop::~EventLoop(){
    delete ep;
    delete threadPool;  //删除 threadPool，避免内存泄漏
}

void EventLoop::loop(){
    while(!quit){
        std::vector<Channel*> chs;
        chs = ep->poll();
        for(auto it = chs.begin();it != chs.end();++it){
            (*it)->handleEvent();
        }
    }
}

void EventLoop::updateChannel(Channel *ch){
    ep->updateChannel(ch);
}

void EventLoop::addThread(std::function<void()> f){
    threadPool->add(f);
}