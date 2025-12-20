#include "Epoll.h"
#include "utils.h"
#include "Channel.h"
#include <strings.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <vector>

#define MAX_EVENTS 1000


Epoll::Epoll() : epfd(-1),events(nullptr){
    epfd = epoll_create(0);
    errif(epfd == -1, "epoll create error");
    events = new epoll_event[MAX_EVENTS];
    bzero(events, sizeof(*events) * MAX_EVENTS);
}

Epoll::~Epoll(){
    if(epfd != -1){
        close(epfd);
        epfd = -1;
    }
    delete [] events;
}

void Epoll::updateChannel(Channel * chan){
    int fd = chan->getFd();
    struct epoll_event ev;
    bzero(&ev, sizeof(ev));
    //用epoll_event封装的channel来快速获取上下文信息
    ev.data.ptr = chan;
    ev.events = chan->getEvents();
    if(!chan->getInEpoll()){
        errif(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1, "epoll add error");
        chan->setInEpoll();        
    }else{
        errif(epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev), "epoll modify error");
    }
}

std::vector<Channel*>Epoll::poll(int timeout){
    std::vector<Channel*> activeChannels;
    int nfds = epoll_wait(epfd, events, MAX_EVENTS, timeout);
    errif(nfds == -1, "epoll wait error");
    for(int i = 0;i<nfds;++i){
        Channel *ch = (Channel*)events[i].data.ptr;
        ch->setRevents(events[i].events);
        activeChannels.push_back(ch);
    }
    return activeChannels;
}