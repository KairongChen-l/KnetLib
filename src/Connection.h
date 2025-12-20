#pragma once

#include <functional>

class EventLoop;
class Socket;
class Channel;
class Connection{
private:
    EventLoop* loop;
    Socket *sock;
    Channel* chan;
    std::function<void(Socket*)> deleteConnectionCallBack;

public:
    Connection(EventLoop* _loop,Socket* _sock);
    ~Connection();

    void echo(int sockfd);
    void setDeleteConnectionCallBack(std::function<void(Socket*)>);
};