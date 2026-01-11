#pragma once

#include <map>
#include <vector>

class EventLoop;
class Acceptor;
class Connection;
class ThreadPool;
class InetAddress;

class Server{
private:
    EventLoop *mainReactor;
    Acceptor *acceptor;
    std::map<int,Connection*> connections;
    std::vector<EventLoop*> subReactors;
    ThreadPool *thpool;
public:
    Server(EventLoop*, const InetAddress& local);
    ~Server();

    void handleReadEvent(int);
    void newConnection(int sockfd, const InetAddress& local, const InetAddress& peer);
    void deleteConnection(int sockfd);
};