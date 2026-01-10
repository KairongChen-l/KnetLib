#pragma once
#include "Buffer.h"
#include <functional>
#include <string>

class EventLoop;
class Socket;
class Channel;
class Connection{
private:
    EventLoop* loop;
    Socket *sock;
    Channel* chan;
    std::function<void(int)> deleteConnectionCallBack;
    std::string *inBuffer; //TODO：放在private域，目前用不了
    Buffer *readBuffer;
public:
    Connection(EventLoop* _loop,Socket* _sock);
    ~Connection();

    void echo(int sockfd);
    void setDeleteConnectionCallBack(std::function<void(int)>);
    void send(int sockfd);

};