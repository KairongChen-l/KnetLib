#include "knetlib/Server.h"
#include "knetlib/Socket.h"
#include "knetlib/Acceptor.h"
#include "knetlib/Connection.h"
#include "knetlib/ThreadPool.h"
#include "knetlib/EventLoop.h"
#include <functional>
#include <thread>


Server::Server(EventLoop *_loop, const InetAddress& local) : mainReactor(_loop), acceptor(nullptr){ 
    acceptor = new Acceptor(mainReactor, local);
    std::function<void(int, const InetAddress&, const InetAddress&)> cb = std::bind(&Server::newConnection, this, 
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
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
    // 先停止所有 EventLoop
    for(auto loop : subReactors){
        if(loop != nullptr){
            loop->quit();
        }
    }
    // 等待 ThreadPool 中的任务完成
    delete thpool;
    // 删除 subReactors 中的 EventLoop 对象，避免内存泄漏
    for(auto loop : subReactors){
        delete loop;
    }
    delete acceptor;
    // 清理所有连接
    for(auto& pair : connections){
        if(pair.second != nullptr){
            delete pair.second;
        }
    }
    connections.clear();
}


void Server::newConnection(int sockfd, const InetAddress& local, const InetAddress& peer){
    if(sockfd != -1){
        //TODO:这里可以设计负载均衡算法
        int random = sockfd % subReactors.size();
        Socket *sock = new Socket(sockfd);
        Connection *conn = new Connection(subReactors[random], sock);
        std::function<void(int)> cb = std::bind(&Server::deleteConnection, this, std::placeholders::_1);
        conn->setDeleteConnectionCallBack(cb);
        connections[sockfd] = conn;
    }
}

void Server::deleteConnection(int sockfd){
    if(sockfd != -1){
        auto it = connections.find(sockfd);
        if(it != connections.end()){
            Connection *conn = it->second;
            connections.erase(it);  // 使用迭代器删除更安全
            // 注意：Connection 的析构函数会关闭 socket 并清理资源
            // 如果 Connection 还在 EventLoop 中被使用，删除可能导致段错误
            // 建议使用 shared_ptr 管理 Connection 的生命周期
            delete conn;
        }
    }
}