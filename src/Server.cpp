#include "Server.h"
#include "Socket.h"
#include "Acceptor.h"
#include "Connection.h"
#include "ThreadPool.h"
#include "EventLoop.h"
#include <functional>
#include <thread>


Server::Server(EventLoop *_loop) : mainReactor(_loop), acceptor(nullptr){ 
    acceptor = new Acceptor(mainReactor);
    std::function<void(Socket*)> cb = std::bind(&Server::newConnection, this, std::placeholders::_1);
    acceptor->setNewConnectionCallback(cb);

    int size = std::thread::hardware_concurrency();
    thpool = new ThreadPool(size);
    for(int i = 0;i<size;++i){
        subReactors.emplace_back(new EventLoop());
    }

    for(int i = 0;i<size;++i){
        std::function<void()> sub_loop = std::bind(&EventLoop::loop, subReactors[i]);
        thpool->add(sub_loop);
    }

}

Server::~Server(){
    delete acceptor;
    delete thpool;
    // 删除 subReactors 中的 EventLoop 对象，避免内存泄漏
    for(auto loop : subReactors){
        delete loop;
    }
}


void Server::newConnection(Socket *sock){
    if(sock->getFd() != -1){
        //TODO:这里可以设计负载均衡算法
        int random = sock->getFd() % subReactors.size();
        Connection *conn = new Connection(subReactors[random],sock);
        std::function<void(int)> cb = std::bind(&Server::deleteConnection, this, std::placeholders::_1);
        conn->setDeleteConnectionCallBack(cb);
        connections[sock->getFd()] = conn;
    }
}

void Server::deleteConnection(int sockfd){
    if(sockfd != -1){
        auto it = connections.find(sockfd);
        if(it != connections.end()){
            Connection *conn = connections[sockfd];
            connections.erase(sockfd);
            // close(sockfd);       //正常
            delete conn;         //会Segmant fault
        }
    }
}